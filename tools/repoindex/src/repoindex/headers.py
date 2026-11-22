"""Heuristic C/C++ header parsing; no filesystem or CLI side effects."""
import re


HEADER_EXTS = {".h", ".hpp", ".hh", ".hxx"}


SKIP_NAME_TOKENS = {"class", "struct", "union", "enum", "final"}


BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)


LINE_COMMENT = re.compile(r"//[^\n]*")


DECL_RE = re.compile(r"^\s*(class|struct|enum|union)\b(.*)$")


INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')


DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)")


VAR_RE = re.compile(
    r"^\s*((?:extern|static|const|constexpr|volatile|inline)\b[^;(){}]*?)"
    r"\b([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(=[^;]*)?;\s*$")


NAME_RE = re.compile(r"[A-Za-z_]\w*")


ENUM_OPEN_RE = re.compile(r"^\s*(?:typedef\s+)?enum\b[^;{]*\{")


ENUMERATOR_RE = re.compile(r"^\s*([A-Za-z_]\w*)\s*(?:=[^,]*)?,?\s*(?:\}.*)?$")


TYPEDEF_RE = re.compile(r"^\s*typedef\b(.*);\s*$")


TYPEDEF_OPEN_RE = re.compile(r"^\s*typedef\s+(?:struct|union|enum)\b[^;]*$")


TYPEDEF_CLOSE_RE = re.compile(r"^\s*\}\s*([A-Za-z_]\w*)\s*(?:,[^;]*)?;\s*$")


FUNC_PTR_RE = re.compile(r"\(\s*(?:[A-Za-z_]\w*\s*::\s*)?\*\s*([A-Za-z_]\w*)\s*\)\s*\(")


PROTOTYPE_RE = re.compile(
    r"^\s*(?:(?:extern|static|inline|virtual|explicit|constexpr|const|unsigned|signed|struct|enum)\s+)*"
    r"[A-Za-z_][\w:<>,]*(?:\s*[*&]+\s*|\s+)[*&\s]*(?:[A-Za-z_]\w*::)*(~?[A-Za-z_]\w*)\s*\(")


NOT_FUNCTIONS = {"typedef", "using", "friend", "if", "for", "while", "switch", "return", "sizeof", "case", "do", "else",
                 "delete", "new", "throw", "decltype", "alignof", "typeof", "defined"}


def strip_comments(src):
    def repl(m):
        return "\n" * m.group(0).count("\n")
    src = BLOCK_COMMENT.sub(repl, src)
    src = LINE_COMMENT.sub("", src)
    return src


def looks_like_macro(token):
    return (token.isupper() or token.endswith("_API") or
            token.startswith("__"))


def symbol_name(rest):
    """Declared identifier from the text after class/struct/enum/union.
    Skips 'enum class', macro decorations (CAMERA_API) and 'final'."""
    head = rest.split("{", 1)[0].split(";", 1)[0].split(":", 1)[0]
    for tok in NAME_RE.findall(head):
        if tok in SKIP_NAME_TOKENS or looks_like_macro(tok):
            continue
        return tok
    return ""


def typedef_name(body):
    """Declared name of a one-line typedef body (text between typedef and ;)."""
    pointer = FUNC_PTR_RE.search(body)
    if pointer:
        return pointer.group(1)
    head = re.sub(r"\[[^\]]*\]\s*$", "", body.split("{", 1)[0] if "}" not in body else body.rsplit("}", 1)[1]).strip()
    names = NAME_RE.findall(head)
    return names[-1] if names else ""


def parse_header(src):
    """-> (symbols, includes)

    symbols:  (name, kind, decl_line) - class/struct/enum/union (incl. forward
              declarations; lines cut at the opening brace, enums kept
              whole), #define macros, exposed global variables, enumerators,
              typedef names, one-line function prototypes and function-pointer
              fields (heuristic, line based).
    includes: (target, system_flag) pairs, in source order.
    """
    symbols = []
    includes = []
    in_enum = False
    pending_typedef = False
    for line in strip_comments(src).splitlines():
        stripped = line.strip()
        mi = INCLUDE_RE.match(line)
        if mi:
            includes.append((mi.group(2).strip(), 1 if mi.group(1) == "<" else 0))
            continue
        md = DEFINE_RE.match(line)
        if md:
            symbols.append((md.group(1), "define", stripped))
            continue
        if stripped.startswith("#"):
            continue
        if in_enum:
            me = ENUMERATOR_RE.match(line)
            if me and not stripped.startswith("}"):
                symbols.append((me.group(1), "enumerator", stripped))
            if "}" in line:
                in_enum = False
                mc = TYPEDEF_CLOSE_RE.match(line)
                if pending_typedef and mc:
                    symbols.append((mc.group(1), "typedef", stripped))
                pending_typedef = False
            continue
        if ENUM_OPEN_RE.match(line):
            pending_typedef = stripped.startswith("typedef")
            m = DECL_RE.match(line)
            name = symbol_name(m.group(2)) if m else ""
            if name:
                symbols.append((name, "enum", stripped))
            tail = line.split("{", 1)[1]
            for part in tail.split("}", 1)[0].split(","):
                me = ENUMERATOR_RE.match(part)
                if me:
                    symbols.append((me.group(1), "enumerator", part.strip()))
            in_enum = "}" not in tail
            if not in_enum:
                pending_typedef = False
            continue
        mt = TYPEDEF_RE.match(line)
        if mt:
            name = typedef_name(mt.group(1))
            if name:
                symbols.append((name, "typedef", stripped))
            continue
        if stripped.startswith("typedef"):
            pointer = FUNC_PTR_RE.search(line)
            if pointer:
                symbols.append((pointer.group(1), "typedef", stripped))
                continue
        if TYPEDEF_OPEN_RE.match(line):
            pending_typedef = True
        mc = TYPEDEF_CLOSE_RE.match(line)
        if mc and pending_typedef:
            symbols.append((mc.group(1), "typedef", stripped))
            pending_typedef = False
        m = DECL_RE.match(line)
        if m:
            kind, rest = m.group(1), m.group(2)
            text = (kind + rest).rstrip()
            if kind in ("class", "struct", "union"):
                text = text.split("{", 1)[0].rstrip()
                # Parameters and uses ("struct foo **out,", "struct foo* x);") are not declarations.
                if "{" not in line and (re.search(r"[*&()=]|,\s*$", text) or
                                        re.match(r"^\w+\s+[A-Za-z_]\w*(?:\s*<[^>]*>)?\s+[A-Za-z_]\w*\s*(\[[^\]]*\])?\s*;", text)):
                    continue
            if text and text != kind:
                name = symbol_name(rest)
                if name:
                    symbols.append((name, kind, text))
            if kind == "enum" and "{" in line:
                in_enum = "}" not in line.split("{", 1)[1]
            continue
        mp = FUNC_PTR_RE.search(line)
        if mp and stripped.endswith((";", ",", "(")) and not stripped.startswith(("return", "typedef")):
            symbols.append((mp.group(1), "member", stripped))
            continue
        mv = VAR_RE.match(line)
        if mv:
            symbols.append((mv.group(2), "var", mv.group(0).strip()))
            continue
        mf = PROTOTYPE_RE.match(line)
        if mf and "=" not in line.split("(", 1)[0] and not stripped.endswith(("{", "}")):
            name = mf.group(1)
            first = NAME_RE.match(stripped)
            if name not in NOT_FUNCTIONS and first and first.group(0) not in NOT_FUNCTIONS:
                symbols.append((name, "function", stripped))
    return symbols, includes
