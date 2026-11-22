"""Complete implementation-text storage and bounded offline retrieval."""
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import time

from .configuration import contained

EXTENSIONS = (".cpp", ".cc")
SCHEMA = "2"
SCHEMAS = ("1", "2")
MAX_TOKENS = 64
# Large bodies also get an index-only segment FTS so that a candidate is loaded
# only when one segment holds every query trigram. Segments overlap by the
# maximum query length, so a literal always lies wholly inside one segment.
SEGMENT_THRESHOLD = 1 << 20
SEGMENT_SIZE = 1 << 18
SEGMENT_OVERLAP = 4096

def normalize(text):
    return text.replace("\r\n", "\n").replace("\r", "\n").replace("\x00", "\ufffd")

def build(db, source, extensions=EXTENSIONS, exclude=()):
    """exclude: tree-relative path prefixes whose files are not content-indexed."""
    source = Path(source).resolve()
    extensions = tuple(sorted(set(extensions)))
    if not extensions or any(not re.fullmatch(r"\.[A-Za-z0-9+]+", ext) for ext in extensions):
        raise ValueError("Invalid implementation extensions")
    exclude = tuple(sorted({prefix.strip("/") for prefix in exclude if prefix.strip("/")}))
    if any(".." in prefix.split("/") for prefix in exclude):
        raise ValueError("Invalid implementation exclusion")
    skip_sql, skip_params = "", []
    for prefix in exclude:
        skip_sql += " AND NOT (path=? OR (path>=? AND path<?))"
        skip_params += [prefix, prefix + "/", prefix + "0"]
    db.execute("DROP TABLE IF EXISTS implementation_fts")
    db.execute("DROP VIEW IF EXISTS implementation_files")
    db.execute("DROP TABLE IF EXISTS implementation_files")
    db.execute("DROP TABLE IF EXISTS implementation_paths")
    db.execute("DROP TABLE IF EXISTS implementation_bodies")
    db.execute("DROP TABLE IF EXISTS implementation_segments_fts")
    db.execute("DROP TABLE IF EXISTS implementation_segments")
    db.execute("CREATE TABLE implementation_segments(id INTEGER PRIMARY KEY, body_id INTEGER NOT NULL)")
    db.execute("CREATE VIRTUAL TABLE implementation_segments_fts USING fts5(text, content='', tokenize='trigram', detail='none', columnsize=0)")
    # Identical bodies (vendored/prebuilt header copies) are stored and indexed once.
    db.execute("CREATE TABLE implementation_bodies(id INTEGER PRIMARY KEY, body_sha256 TEXT UNIQUE NOT NULL, body TEXT NOT NULL, lines INTEGER NOT NULL)")
    db.execute("CREATE TABLE implementation_paths(id INTEGER PRIMARY KEY, path TEXT UNIQUE, size INTEGER, mtime INTEGER, source_sha256 TEXT, body_id INTEGER NOT NULL REFERENCES implementation_bodies(id), normalized INTEGER)")
    bodies = {}
    placeholders = ",".join("?" for _ in extensions)
    selection = "FROM vfiles WHERE ext IN (" + placeholders + ")" + skip_sql
    expected = db.execute("SELECT count(*),coalesce(sum(size),0) " + selection, extensions + tuple(skip_params)).fetchone()
    rows = db.execute("SELECT path,size,mtime " + selection + " ORDER BY path", extensions + tuple(skip_params))
    count = total = replacements = normalized_files = 0
    for rel, size, mtime in rows:
        unresolved = source / rel
        if unresolved.is_symlink() or not stat.S_ISREG(unresolved.lstat().st_mode):
            raise ValueError("Implementation is not a regular file: " + rel)
        path = contained(source, rel)
        with os.fdopen(os.open(path, os.O_RDONLY | os.O_NOFOLLOW), "rb") as stream:
            before = os.fstat(stream.fileno())
            raw = stream.read()
            after = os.fstat(stream.fileno())
        if (before.st_size, int(before.st_mtime)) != (size, int(mtime)) or len(raw) != size or (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise ValueError("Implementation changed since file snapshot: " + rel)
        try:
            decoded = raw.decode("utf-8")
        except UnicodeDecodeError:
            decoded = raw.decode("utf-8", errors="replace")
            replacements += 1
        body = normalize(decoded)
        changed = body != decoded or "\ufffd" in decoded
        normalized_files += int(changed)
        line_count = body.count("\n") + int(bool(body) and not body.endswith("\n"))
        raw_sha = hashlib.sha256(raw).hexdigest()
        text_sha = hashlib.sha256(body.encode("utf-8")).hexdigest()
        body_id = bodies.get(text_sha)
        if body_id is None:
            body_id = db.execute("INSERT INTO implementation_bodies(body_sha256,body,lines) VALUES(?,?,?)",
                                 (text_sha, body, line_count)).lastrowid
            bodies[text_sha] = body_id
            if len(body) > SEGMENT_THRESHOLD:
                for start in range(0, len(body), SEGMENT_SIZE):
                    segment = db.execute("INSERT INTO implementation_segments(body_id) VALUES(?)", (body_id,)).lastrowid
                    db.execute("INSERT INTO implementation_segments_fts(rowid,text) VALUES(?,?)",
                               (segment, body[start:start + SEGMENT_SIZE + SEGMENT_OVERLAP]))
        db.execute("INSERT INTO implementation_paths(path,size,mtime,source_sha256,body_id,normalized) VALUES(?,?,?,?,?,?)",
                   (rel, size, int(mtime), raw_sha, body_id, int(changed)))
        count += 1
        total += size
        if count % 1000 == 0:
            db.commit()
        if count % 10000 == 0:
            print("  ... %d complete implementation files" % count, flush=True)
    db.commit()
    if (count, total) != expected:
        raise ValueError("Incomplete implementation read; previous bundle retained")
    # Whole-file documents preserve multiline/cross-block matches; no body sampling.
    db.execute("CREATE INDEX implementation_paths_body ON implementation_paths(body_id, path)")
    # Read-only compatibility view: one row per path, as in schema 1.
    db.execute("CREATE VIEW implementation_files AS SELECT p.id AS id, p.path AS path, p.size AS size, p.mtime AS mtime, "
               "p.source_sha256 AS source_sha256, b.body_sha256 AS body_sha256, b.body AS body, b.lines AS lines, "
               "p.normalized AS normalized FROM implementation_paths p JOIN implementation_bodies b ON b.id=p.body_id")
    db.execute("CREATE VIRTUAL TABLE implementation_fts USING fts5(body, content='implementation_bodies', content_rowid='id', tokenize='trigram', detail='none', columnsize=0)")
    print("Building implementation trigram postings for %d unique bodies of %d files ..." % (len(bodies), count), flush=True)
    last = [time.monotonic()]
    def progress():
        if time.monotonic() - last[0] >= 20:
            print("  ... implementation postings still building", flush=True)
            last[0] = time.monotonic()
        return 0
    db.set_progress_handler(progress, 100000)
    try:
        db.execute("INSERT INTO implementation_fts(implementation_fts) VALUES('rebuild')")
        db.execute("INSERT INTO implementation_fts(implementation_fts) VALUES('optimize')")
    finally:
        db.set_progress_handler(None, 0)
    meta = {"implementation_schema": SCHEMA, "implementation_backend": "trigram",
            "implementation_extensions": json.dumps(extensions), "implementation_exclude": json.dumps(exclude),
            "implementation_count": str(count),
            "implementation_segmented_bodies": str(db.execute("SELECT count(DISTINCT body_id) FROM implementation_segments").fetchone()[0]),
            "implementation_bytes": str(total), "implementation_unique_bodies": str(len(bodies)),
            "implementation_decode_errors": str(replacements),
            "implementation_normalized_files": str(normalized_files)}
    db.executemany("INSERT OR REPLACE INTO meta(key,value) VALUES(?,?)", meta.items())
    db.commit()
    return meta

def available(db):
    value = db.execute("SELECT value FROM meta WHERE key='implementation_schema'").fetchone()
    if not value or value[0] not in SCHEMAS:
        raise ValueError("No implementation-content index in this snapshot; explicitly run workspace implement or workspace index once")
    return value[0]

def scope_sql(prefixes):
    clauses, params = [], []
    for prefix in prefixes:
        if not prefix:
            clauses.append("1")
        else:
            clauses.append("(path=? OR (path>=? AND path<?))")
            params.extend([prefix, prefix + "/", prefix + "0"])
    return "(" + " OR ".join(clauses) + ")", params

def match_expression(pattern):
    """AND of distinct overlapping trigrams (detail=none tokens are <=3 codepoints).

    Using every distinct trigram, up to MAX_TOKENS spread over the literal, keeps
    false-positive candidates low; exact matches are verified in Python, so no
    true match can be omitted."""
    if len(pattern) < 3:
        return None
    grams = list(dict.fromkeys(pattern[i:i+3] for i in range(len(pattern) - 2)))
    if len(grams) > MAX_TOKENS:
        step = (len(grams) - 1) / (MAX_TOKENS - 1)
        grams = [grams[round(i * step)] for i in range(MAX_TOKENS)]
    return " AND ".join('"' + token.replace('"', '""') + '"' for token in sorted(set(grams)))

def candidates_sql(pattern, prefixes):
    """Schema 1: one candidate row per path."""
    where, params = scope_sql(prefixes)
    match = match_expression(pattern)
    if match:
        where += " AND id IN (SELECT rowid FROM implementation_fts WHERE implementation_fts MATCH ?)"
        params.append(match)
    return "SELECT id,path FROM implementation_files WHERE " + where + " ORDER BY path", params

def body_candidates(db, pattern, prefixes):
    """Schema 2: (body, in-scope paths) in order of each body's first path.

    Scope is checked through the (body_id, path) index before a body is loaded,
    so scoped queries never read or verify out-of-scope documents."""
    where, params = scope_sql(prefixes)
    match = match_expression(pattern)
    if match:
        ids = db.execute("SELECT rowid FROM implementation_fts WHERE implementation_fts MATCH ? ORDER BY rowid", (match,))
    else:
        ids = db.execute("SELECT id FROM implementation_bodies ORDER BY id")
    path_sql = "SELECT path FROM implementation_paths WHERE body_id=? AND " + where + " ORDER BY path"
    segmented = matched = set()
    if match and db.execute("SELECT 1 FROM sqlite_master WHERE name='implementation_segments'").fetchone():
        segmented = {r[0] for r in db.execute("SELECT DISTINCT body_id FROM implementation_segments")}
        if segmented:
            matched = {r[0] for r in db.execute(
                "SELECT DISTINCT body_id FROM implementation_segments WHERE id IN "
                "(SELECT rowid FROM implementation_segments_fts WHERE implementation_segments_fts MATCH ?)", (match,))}
    for (body_id,) in ids:
        if body_id in segmented and body_id not in matched:
            continue
        paths = [r[0] for r in db.execute(path_sql, [body_id] + params)]
        if not paths:
            continue
        body = db.execute("SELECT body FROM implementation_bodies WHERE id=?", (body_id,)).fetchone()[0]
        if pattern in body:
            yield body, paths

def excerpt(body, offset, maximum):
    start = body.rfind("\n", 0, offset) + 1
    end = body.find("\n", offset)
    end = len(body) if end < 0 else end
    left = max(start, offset - min(80, maximum // 3))
    right = min(end, left + maximum)
    return ("…" if left > start else "") + body[left:right] + ("…" if right < end else ""), left > start or right < end

def legacy_candidates(db, pattern, prefixes):
    sql, params = candidates_sql(pattern, prefixes)
    for file_id, path in db.execute(sql, params):
        body = db.execute("SELECT body FROM implementation_files WHERE id=?", (file_id,)).fetchone()[0]
        yield body, [path]

def search(db, pattern, prefixes, context=0, max_chars=400, collapse=True):
    """Literal matches; identical file copies collapse into one record with
    ``copies`` (other path count) and ``other_paths`` (first few) unless
    collapse=False, which yields one record per path as before."""
    schema = available(db)
    pattern = normalize(pattern)
    if not pattern or len(pattern) > 4096 or not 0 <= context <= 5 or not 16 <= max_chars <= 2000:
        raise ValueError("Require 1..4096 query characters, context 0..5 and snippet size 16..2000")
    source = body_candidates(db, pattern, prefixes) if schema == "2" else legacy_candidates(db, pattern, prefixes)
    for body, paths in source:
        groups = [paths] if collapse else [[path] for path in paths]
        for group in groups:
            yield from matches(body, group, pattern, context, max_chars)

def matches(body, paths, pattern, context, max_chars):
    path = paths[0]
    offset = body.find(pattern)
    while offset >= 0:
        line = body.count("\n", 0, offset) + 1
        end_line = body.count("\n", 0, offset + len(pattern) - 1) + 1
        text, clipped = excerpt(body, offset, max_chars)
        result = {"path": path, "line": line, "end_line": end_line,
                  "column": offset - body.rfind("\n", 0, offset), "text": text}
        if clipped:
            result["truncated"] = True
        if len(paths) > 1:
            result.update(copies=len(paths) - 1, other_paths=paths[1:4])
        if context:
            left = body.rfind("\n", 0, offset) + 1
            first_line = line
            for _ in range(context):
                if left == 0:
                    break
                left = body.rfind("\n", 0, left - 1) + 1
                first_line -= 1
            right = body.find("\n", offset + len(pattern))
            right = len(body) if right < 0 else right
            for _ in range(context):
                following = body.find("\n", right + 1)
                right = len(body) if following < 0 else following
            block = body[left:right]
            result.update(context_start_line=first_line, context=block[:1600])
            if len(block) > 1600:
                result["context_truncated"] = True
        yield result
        # One record per starting line, even when the literal repeats there.
        following = body.find("\n", offset)
        offset = -1 if following < 0 else body.find(pattern, following + 1)

def validate_range(line, limit, max_chars):
    if line < 1 or not 1 <= limit <= 200 or not 16 <= max_chars <= 2000:
        raise ValueError("Require line >=1, 1..200 lines and line width 16..2000")


def read(db, path, line=1, limit=40, max_chars=400):
    validate_range(line, limit, max_chars)
    available(db)
    row = db.execute("SELECT body,source_sha256,lines FROM implementation_files WHERE path=?", (path,)).fetchone()
    if row is None:
        return None
    body, sha, total = row
    if line > total:
        return None
    lines = body.split("\n")
    chosen = lines[line-1:min(total, line-1+limit)]
    clipped = any(len(text) > max_chars for text in chosen)
    shown = [text[:max_chars] + ("…" if len(text) > max_chars else "") for text in chosen]
    return {"path": path, "start_line": line, "end_line": line + len(chosen) - 1,
            "total_lines": total, "text": "\n".join(shown), "truncated": clipped,
            "source_sha256": sha}
