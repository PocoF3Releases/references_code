#define LOG_TAG "MIUINDBG"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <paths.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <private/android_filesystem_config.h>
#include <cutils/properties.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <UniquePtr.h>
#include <openssl/aes.h>
#include <string>
#include <vector>
#include "tinyxml2.h"
#include "tombstoneparser.h"
#include "ndbglog.h"
using namespace tinyxml2;


#define WL_XML_RULE                         "rule"
#define WL_XML_RULE_ID                      "id"
#define WL_XML_RULE_PN                      "pn"
#define WL_XML_RULE_BT                      "bt"
#define WL_XML_RULE_MAPS                    "maps"
#define WL_XML_RULE_TOMB                    "tomb"
#define WL_XML_RULE_PS                      "ps"
#define WL_XML_RULE_FILE                    "file"
#define WL_XML_RULE_FILE_PATH               "path"
#define WL_XML_RULE_FILE_KEYWORD            "keyword"
#define WL_XML_RULE_FILE_ACTION             "action"
#define WL_XML_RULE_FILE_ACTION_DUMP        "dump"
#define WL_XML_RULE_FILE_ACTION_STAT        "stat"
#define WL_XML_RULE_MLOG                    "mlog"
#define WL_XML_RULE_SLOG                    "slog"
#define WL_XML_RULE_ELOG                    "elog"
#define WL_XML_RULE_RLOG                    "rlog"
#define WL_XML_RULE_KLOG                    "klog"
#define WL_XML_RULE_JSTK                    "jstack"
#define WL_XML_RULE_JSTK_TYPE               "type"
#define WL_XML_RULE_JSTK_TYPE_ALL           "all"
#define WL_XML_RULE_CORE                    "core"
#define WL_XML_RULE_CORE_TYPE               "type"
#define WL_XML_RULE_CORE_TYPE_FULL          "full"
#define WL_XML_RULE_LSOF                    "lsof"
#define WL_XML_RULE_SPEC                    "spec"

#define MIUI_NATIVE_DEBUG_ROOT_DIR          "/data/miuilog/stability/nativecrash"
#define MIUI_NATIVE_DEBUG_ZIP_DIR           MIUI_NATIVE_DEBUG_ROOT_DIR"/zip"
#define MIUI_NATIVE_DEBUG_CORE_DIR          MIUI_NATIVE_DEBUG_ROOT_DIR"/core"
#ifdef NDBG_LOCAL
#define MIUI_NATIVE_DEBUG_RULES_XML         MIUI_NATIVE_DEBUG_ROOT_DIR"/rules_local.xml"
#else
#define MIUI_NATIVE_DEBUG_RULES_XML         MIUI_NATIVE_DEBUG_ROOT_DIR"/rules.xml"
#endif
#define MIUI_NATIVE_DEBUG_LOCK              MIUI_NATIVE_DEBUG_ROOT_DIR"/lock"

#define MIUI_NATIVE_FREE_SPACE_MIN          (unsigned long)0x40000000
#if defined(__LP64__)
#define MIUI_NATIVE_FREE_SPACE_MAX          (unsigned long)(0x180000000)  //6G
#else
#define MIUI_NATIVE_FREE_SPACE_MAX          (unsigned long)(0xC0000000)  //3G
#endif

#define RULE_DUMP_ENABLE            (unsigned char)1

/*none default full*/
#define RULE_CORE_TYPE_DEFAULT      (unsigned char)1
#define RULE_CORE_TYPE_FULL         (unsigned char)2

/*none current all*/
#define RULE_JSTK_TYPE_CURRENT      (unsigned char)1
#define RULE_JSTK_TYPE_ALL          (unsigned char)2

/*none path keyword*/
#define RULE_FILE_TYPE_PATH        (unsigned char)1
#define RULE_FILE_TYPE_KEYWORD     (unsigned char)2

/*none stat dump*/
#define RULE_FILE_ACTION_STAT       (unsigned char)1
#define RULE_FILE_ACTION_DUMP       (unsigned char)2

#define RULE_UID_NONE               (char*)0
#define RULE_UID_SYS                (char*)1
#define RULE_UID_ALL                (char*)2
#define RULE_UID_MAX                (char*)3

typedef struct rule_file {
    struct rule_file*   next;
    char*               p_str;
    unsigned char       c_type;
    unsigned char       c_action;
} RULE_FILE_T;

typedef struct rule_dump_info_t {
    unsigned int       core:2;
    unsigned int       jstk:2;
    unsigned int       mlog:1;
    unsigned int       slog:1;
    unsigned int       elog:1;
    unsigned int       rlog:1;
    unsigned int       klog:1;
    unsigned int       maps:1;
    unsigned int       tomb:1;
    unsigned int       ps:1;
    unsigned int       lsof:1;
    unsigned int       spec:1;
    RULE_FILE_T*       p_file;
} RULE_DI_T;

typedef struct rule_t {
    struct rule_t*      next;
    int                 i_id;
    char*               p_pn;
    char*               p_bt;
    RULE_DI_T           t_di;
} RULE_T;

static bool s_dump_enable = true;
static RULE_T* s_rules_ptr = NULL;
static long s_rules_mtime = 0;
static unsigned long s_core_limit = MIUI_NATIVE_FREE_SPACE_MIN/4*3;

static void dump_core(pid_t pid, pid_t tid, RULE_T* rule);

static char* s_tombstone_path = NULL;

#ifdef NDEBUG
#define  print_rule(x)
#else
static void print_rule(RULE_T* rule)
{
    MILOGI("\t<" WL_XML_RULE ">");
    MILOGI("\t\t<" WL_XML_RULE_ID ">%08d</" WL_XML_RULE_ID ">",rule->i_id);

    const char* package_name = NULL;

    if (rule->p_pn == RULE_UID_ALL) {
        package_name = "A";
    } else if (rule->p_pn == RULE_UID_SYS) {
        package_name = "S";
    } else {
        package_name = rule->p_pn;
    }

    MILOGI("\t\t<" WL_XML_RULE_PN ">%s</" WL_XML_RULE_PN ">",package_name);

    if (rule->p_bt) MILOGI("\t\t<" WL_XML_RULE_BT ">%s</" WL_XML_RULE_BT ">", rule->p_bt);

    if (rule->t_di.maps) MILOGI("\t\t<" WL_XML_RULE_MAPS "/>");
    if (rule->t_di.tomb) MILOGI("\t\t<" WL_XML_RULE_TOMB "/>");
    if (rule->t_di.ps) MILOGI("\t\t<" WL_XML_RULE_PS "/>");
    if (rule->t_di.lsof) MILOGI("\t\t<" WL_XML_RULE_LSOF "/>");
    if (rule->t_di.spec) MILOGI("\t\t<" WL_XML_RULE_SPEC "/>");
    if (rule->t_di.mlog) MILOGI("\t\t<" WL_XML_RULE_MLOG "/>");
    if (rule->t_di.slog) MILOGI("\t\t<" WL_XML_RULE_SLOG "/>");
    if (rule->t_di.elog) MILOGI("\t\t<" WL_XML_RULE_ELOG "/>");
    if (rule->t_di.rlog) MILOGI("\t\t<" WL_XML_RULE_RLOG "/>");
    if (rule->t_di.klog) MILOGI("\t\t<" WL_XML_RULE_KLOG "/>");

    if (rule->t_di.core == RULE_CORE_TYPE_DEFAULT) {
         MILOGI("\t\t<" WL_XML_RULE_CORE "/>");
    } else if (rule->t_di.core == RULE_CORE_TYPE_FULL) {
         MILOGI("\t\t<" WL_XML_RULE_CORE " " WL_XML_RULE_CORE_TYPE "=\"" WL_XML_RULE_CORE_TYPE_FULL "\"/>");
    }

    if (rule->t_di.jstk == RULE_JSTK_TYPE_CURRENT) {
         MILOGI("\t\t<" WL_XML_RULE_JSTK "/>");
    } else if (rule->t_di.jstk == RULE_JSTK_TYPE_ALL) {
         MILOGI("\t\t<" WL_XML_RULE_JSTK " " WL_XML_RULE_JSTK_TYPE "=\"" WL_XML_RULE_JSTK_TYPE_ALL "\"/>");
    }

    RULE_FILE_T* p_file = rule->t_di.p_file;
    if (p_file) {
        const char* action = NULL;
        const char* type = NULL;

        MILOGI("\t\t<" WL_XML_RULE_FILE ">");

        while (p_file) {
            if (p_file->c_type == RULE_FILE_TYPE_PATH) {
                type = WL_XML_RULE_FILE_PATH;
            } else if (p_file->c_type == RULE_FILE_TYPE_KEYWORD) {
                type = WL_XML_RULE_FILE_KEYWORD;
            } else {
                MILOGI("\t\t\tunknown file type!");
            }

            if (p_file->c_action == RULE_FILE_ACTION_STAT) {
                action = WL_XML_RULE_FILE_ACTION_STAT;
            } else if (p_file->c_action == RULE_FILE_ACTION_DUMP) {
                action = WL_XML_RULE_FILE_ACTION_DUMP;
            } else {
                MILOGI("\t\t\tunknown file action!");
            }

            MILOGI("\t\t\t<%s " WL_XML_RULE_FILE_ACTION "=\"%s\">%s</%s>",type,action,p_file->p_str,type);

            p_file = p_file->next;
        }
        MILOGI("\t\t</" WL_XML_RULE_FILE ">");
    }
    MILOGI("\t</" WL_XML_RULE ">");
}
#endif

static RULE_T* alloc_rule()
{
    RULE_T* rule = (RULE_T*)malloc(sizeof(RULE_T));
    if (rule) {
        rule->next = NULL;
        rule->i_id = -1;
        rule->p_pn = RULE_UID_NONE;
        rule->p_bt = NULL;
        memset(&rule->t_di,0,sizeof(rule->t_di));
    }
    return rule;
}

static void free_rule(RULE_T* rule)
{
    if (!rule) { return; }

    if (rule->p_pn >= RULE_UID_MAX) {
        free(rule->p_pn);
    }

    if (rule->p_bt) {
        free(rule->p_bt);
    }
    RULE_FILE_T* p_file = rule->t_di.p_file;
    RULE_FILE_T* tmp;
    while (p_file) {
        if (p_file->p_str)
            free(p_file->p_str);
        tmp = p_file;
        p_file = p_file->next;
        free(tmp);
    }

    free(rule);
}

static void parse_elem(XMLElement* elem, RULE_T* rule)
{
    const char* name_ptr = elem->Name();
    int name_len = strlen(name_ptr);

    if (!name_ptr || !name_len || !rule) return;

    int text_len = 0;
    const char* text_ptr = elem->GetText();
    if (text_ptr) {
        text_len = strlen(text_ptr);
    }

    if (!strcmp(name_ptr,WL_XML_RULE_ID)) {
        if (text_len == 8) {
            rule->i_id = (unsigned int)atoi(text_ptr);
        }
    } else if (!strcmp(name_ptr,WL_XML_RULE_PN)) {
        if (text_len == 1) {
            char ch = text_ptr[0];
            if (ch == 'A' || ch == 'a') {
                rule->p_pn = RULE_UID_ALL;
            } else if (ch == 'S' || ch == 's') {
                rule->p_pn = RULE_UID_SYS;
            }
        } else {
            rule->p_pn = strdup(text_ptr);
        }
    } else if (!strcmp(name_ptr,WL_XML_RULE_BT)) {
        rule->p_bt = strdup(text_ptr);
    } else if(!strcmp(name_ptr,WL_XML_RULE_FILE)) {
        const char* p_attr = NULL;

        XMLElement* felem = elem->FirstChildElement();
        for(;felem;felem = felem->NextSiblingElement()) {
            name_ptr = felem->Name();
            text_ptr = felem->GetText();
            p_attr = felem->Attribute(WL_XML_RULE_FILE_ACTION, NULL);

            if (NULL == text_ptr || NULL == name_ptr || NULL == p_attr) {
                continue;
            }
            RULE_FILE_T* p_file = (RULE_FILE_T*)malloc(sizeof(RULE_FILE_T));

            if (!strcmp(name_ptr, WL_XML_RULE_FILE_PATH)) {
                p_file->c_type = RULE_FILE_TYPE_PATH;
            } else if (!strcmp(name_ptr, WL_XML_RULE_FILE_KEYWORD)) {
                p_file->c_type = RULE_FILE_TYPE_KEYWORD;
            } else {
                free(p_file);
                continue;
            }

            if (!strcmp(p_attr, WL_XML_RULE_FILE_ACTION_DUMP)) {
                p_file->c_action = RULE_FILE_ACTION_DUMP;
            } else if (!strcmp(p_attr, WL_XML_RULE_FILE_ACTION_STAT)) {
                p_file->c_action = RULE_FILE_ACTION_STAT;
            } else {
                free(p_file);
                continue;
            }
            p_file->p_str = strdup(text_ptr);
            p_file->next = rule->t_di.p_file;
            rule->t_di.p_file = p_file;
        }
    } else if (!strcmp(name_ptr, WL_XML_RULE_MLOG)) {
        rule->t_di.mlog = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_SLOG)) {
        rule->t_di.slog = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_ELOG)) {
        rule->t_di.elog = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_RLOG)) {
        rule->t_di.rlog = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_KLOG)) {
        rule->t_di.klog = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_SPEC)) {
        rule->t_di.spec = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_MAPS)) {
        rule->t_di.maps = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_TOMB)) {
        rule->t_di.tomb = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_PS)) {
        rule->t_di.ps = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_LSOF)) {
        rule->t_di.lsof = RULE_DUMP_ENABLE;
    } else if (!strcmp(name_ptr, WL_XML_RULE_CORE)) {
        rule->t_di.core = RULE_CORE_TYPE_DEFAULT;
        const char* p_attr = elem->Attribute(WL_XML_RULE_CORE_TYPE, NULL);
        if (p_attr) {
            if (!strcmp(p_attr,WL_XML_RULE_CORE_TYPE_FULL)) {
                rule->t_di.core = RULE_CORE_TYPE_FULL;
            }
        }
    } else if (!strcmp(name_ptr, WL_XML_RULE_JSTK)) {
        rule->t_di.jstk = RULE_JSTK_TYPE_CURRENT;
        const char* p_attr = elem->Attribute(WL_XML_RULE_JSTK_TYPE, NULL);
        if (p_attr) {
            if (!strcmp(p_attr, WL_XML_RULE_JSTK_TYPE_ALL)) {
                rule->t_di.jstk = RULE_JSTK_TYPE_ALL;
            }
        }
    }
}

#define SPEC_TAG "MiuiNativeDebug"
static bool fetch_rules(char** rule_str_decoded_p) {
    MILOGD("start loading rules...");
    FILE *fp;
    if ((fp = fopen(MIUI_NATIVE_DEBUG_RULES_XML, "rb")) == NULL) {
        MILOGE("can not open rules file!");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t flen = ftell(fp);
#ifdef NDBG_LOCAL
    char* ptr =  (char*)malloc(flen + 1);
    rewind(fp);
    fread(ptr, 1, flen, fp);
    fclose(fp);
    *(ptr + flen) = 0;
    *rule_str_decoded_p = ptr;
#else
    char* rule_str_decoded = (char*) calloc(1, flen + AES_BLOCK_SIZE);
    if ((flen & (AES_BLOCK_SIZE - 1)) || !rule_str_decoded) {
        MILOGE("malloc error! %zd", flen);
        fclose(fp);
        return false;
    }
    rewind(fp);

    unsigned char iv_dec[AES_BLOCK_SIZE] = SPEC_TAG;
    for (int i = 0; i != AES_BLOCK_SIZE; ++i) {
        if (iv_dec[i] == 'i') {
            iv_dec[i] = 'u';
        } else if (iv_dec[i] == 'u') {
            iv_dec[i] = 'e';
        } else if (iv_dec[i] == 'e') {
            iv_dec[i] = 'o';
        }
    }
    iv_dec[AES_BLOCK_SIZE - 1] = 'X';

    char version[PROPERTY_VALUE_MAX];
    property_get("ro.build.version.incremental", version, "unknown");

    unsigned char aes_key[AES_BLOCK_SIZE];
    int aes_key_index = 0;
    for (int i = 0; version[i] != '\0' && aes_key_index != AES_BLOCK_SIZE; ++i) {
        if ('0' <= version[i] && version[i] <= '9') {
            aes_key[aes_key_index] = version[i];
            ++ aes_key_index;
        }
    }
    for (int i = 0; iv_dec[i] != '\0' && aes_key_index != AES_BLOCK_SIZE; ++i) {
        aes_key[aes_key_index] = iv_dec[i];
        ++ aes_key_index;
    }

    AES_KEY dec_key;
    AES_set_decrypt_key(aes_key, AES_BLOCK_SIZE * 8, &dec_key);
    char* writed_ptr = rule_str_decoded;
    unsigned char switch_buf;
    unsigned char buffer[AES_BLOCK_SIZE];
    unsigned char decbuffer[AES_BLOCK_SIZE];

    fread(buffer, 1, AES_BLOCK_SIZE, fp);
    if (memcmp(buffer, SPEC_TAG, AES_BLOCK_SIZE)) {
        MILOGE("Format is wrong!");
        return false;
    }

    while (fread(buffer, 1, AES_BLOCK_SIZE, fp)) {
        for (int i = 0; i != AES_BLOCK_SIZE / 2; ++ i) {
            switch_buf = buffer[i];
            buffer[i] = buffer[AES_BLOCK_SIZE - 1 - i];
            buffer[AES_BLOCK_SIZE - 1 - i] = switch_buf;
        }
        AES_cbc_encrypt(buffer, decbuffer, AES_BLOCK_SIZE, &dec_key, iv_dec, AES_DECRYPT);
        memcpy(writed_ptr, decbuffer, AES_BLOCK_SIZE);
        writed_ptr += AES_BLOCK_SIZE;
    }
    *rule_str_decoded_p = rule_str_decoded;
    MILOGD("load rules successfully!");
#endif
    return true;
}

static void build_rules()
{
    if (s_rules_ptr) return;

    XMLDocument doc;
    XMLElement* root;

    char* rule_str_decoded;
    if (!fetch_rules(&rule_str_decoded)) {
        return;
    }

    doc.Parse(rule_str_decoded);
    free(rule_str_decoded);

    root = doc.RootElement();

    if (!root) {
        MILOGE("root element is not found!");
        return;
    }

    XMLElement* ruleElem = root->FirstChildElement();
    while (ruleElem) {
        if (!strcmp(ruleElem->Name(),WL_XML_RULE)) {
            RULE_T* rule = alloc_rule();

            if (!rule) break;

            XMLElement* elem = ruleElem->FirstChildElement();
            while (elem) {
                parse_elem(elem,rule);
                elem = elem->NextSiblingElement();
            }

            print_rule(rule);

            if (rule->p_pn > RULE_UID_MAX && strstr(rule->p_pn,"debuggerd")) {
                dump_core(getpid(),gettid(),rule);
                free_rule(rule);
            } else if (rule->p_pn != RULE_UID_NONE && rule->i_id != -1) {
                rule->next = s_rules_ptr;
                s_rules_ptr = rule;
            } else {
                free_rule(rule);
            }
        }
        ruleElem = ruleElem->NextSiblingElement();
    }
}

static void distroy_rules()
{
    RULE_T* rule = s_rules_ptr;
    RULE_T* tmp;

    while (rule) {
        tmp = rule;
        rule = rule->next;
        free_rule(tmp);
    }
    s_rules_ptr = NULL;
}

static void get_proc_name(pid_t pid, char* buf, int buf_len)
{
    char path[PATH_MAX];
    int fd,len;

    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    fd = open(path, O_RDONLY);
    if (fd == 0) {
        len = 0;
    } else {
        len = read(fd, buf, buf_len-1);
        close(fd);
        if (len < 0) len = 0;
    }
    buf[len] = 0;
}

static uid_t get_proc_uid(pid_t pid)
{
    char path[32];
    struct stat stats;

    snprintf(path, sizeof(path), "/proc/%d", pid);
    stat(path, &stats);

    return stats.st_uid;

}

static int bt_atoi(char* start)
{
    int ret = atoi(start);

    if (ret == 0) {
        while (*start == ' ') {
            start++;
        }

        if (*start != '0') {
            ret = -1;
        }
    }

    return ret;
}

static char* parse_bt_line(char* bt, size_t* pBegin, size_t* pEnd)
{

    if (*bt == '#') {
        size_t len = strlen(bt);
        char* hash = strchr(bt + 1, '#');

        if (!hash || (size_t)(hash - bt) >= len - 1) {
            return bt;
        }
        int begin,end;

        char* dot = strchr(bt + 1, '.');

        begin = bt_atoi(bt + 1);

        if (!dot || dot > hash) {
            end = begin;
        } else {
            end = bt_atoi(dot + 1);

            if (end >=0 && begin > end) {
                int tmp = begin;
                begin = end;
                end = tmp;
            }
        }

        if (begin >= 0) {
            *pBegin = (size_t)begin;
        }

        if (end >= 0 && (size_t)end < *pEnd) {
            *pEnd = (size_t)end;
        }
        return hash + 1;
    }
    return bt;
}

static int split_path(const char *path,  std::string delimiter, std::vector<std::string>& paths)
{
    if (!path) {
        return 0;
    }

    std::string str(path);
    if (str.empty()) {
        return 0;
    }

    std::string tmp;
    std::string::size_type pos_begin = str.find_first_not_of(delimiter);
    std::string::size_type comma_pos = 0;

    while (pos_begin != std::string::npos) {
        comma_pos = str.find(delimiter, pos_begin);
        if (comma_pos != std::string::npos) {
            tmp = str.substr(pos_begin, comma_pos - pos_begin);
            pos_begin = comma_pos + delimiter.length();
        } else {
            tmp = str.substr(pos_begin);
            pos_begin = comma_pos;
        }

        if (!tmp.empty()) {
            paths.push_back(tmp);
            tmp.clear();
        }
    }
    return 0;
}

static bool is_positive_digit_string(std::string str) {
    if (str.empty()) return false;

    for(size_t i = 0; i < str.size(); i++) {
        if (str.at(i) > '9' || str.at(i) < '0') return false;
    }

    return true;
}

static RULE_T* match_rules(pid_t pid, pid_t tid)
{
    TombstoneParser tp = TombstoneParser(pid, tid);

    if (tp.getTombFilePath().isEmpty()) {
        MILOGE("cannot find tombstone file!");
        return NULL;
    }

    RULE_T* matched = NULL;
    size_t frames = tp.getBacktraces().size();

    const char* abortMsg = tp.getAbortMsg().string();

    for(RULE_T* rule = s_rules_ptr; rule && !matched; rule = rule->next) {
        if (rule->p_pn == RULE_UID_ALL
        || (rule->p_pn == RULE_UID_SYS && tp.isSystemApp())
        || (rule->p_pn > RULE_UID_MAX && !strcmp(tp.getProcessName().string(), rule->p_pn))) {
            if (rule->p_bt) {
                if (rule->p_bt[0] == '!') {
                    if (abortMsg && strstr(abortMsg, &rule->p_bt[1])) {
                        matched = rule;
                        break;
                    }
                } else if(rule->p_bt[0] == '#') {
                    if (frames == 0) continue;

                    size_t begin = 0;
                    size_t end = frames - 1;
                    char* p_bt = parse_bt_line(rule->p_bt, &begin, &end);
                    if (end > frames - 1) continue;

                    for (size_t i = begin; i <= end; i++) {
                        if (strstr(tp.getBacktraces().itemAt(i).string(), p_bt)) {
                            matched = rule;
                            break;
                        }
                    }
                } else if(rule->p_bt[0] == '*') {
                    if (frames == 0) continue;

                    size_t begin = 0;
                    size_t end = frames - 1;
                    char *bt = rule->p_bt + 1;

                    bt = parse_bt_line(bt, &begin, &end);
                    if (end > frames - 1) continue;

                    std::vector<std::string> paths;
                    split_path(bt, ":", paths);
                    size_t size = paths.size();
                    const char *match_bt;

                    for (size_t i = begin; i <= end; i++) {
                        if (size > 1 && size == (end - begin + 1)) {
                            match_bt = paths[i - begin].c_str();
                        } else if (size == 1 && size <= (end - begin + 1)) {
                            match_bt = paths[0].c_str();
                        } else {
                            break;
                        }
                        if (strstr(tp.getBacktraces().itemAt(i).string(), match_bt)) {
                            matched = rule;
                        } else {
                            matched = NULL;
                            break;
                        }
                    }
                }  else if(rule->p_bt[0] == '+') {
                    char *bt = rule->p_bt + 2;
                    std::vector<std::string> bt_paths;
                    std::vector<std::string> want_frames;
                    std::vector<std::string> want_bts;

                    split_path(bt, "#", bt_paths);
                    if (bt_paths.size() != 2) continue;

                    split_path(bt_paths[0].c_str(), ".", want_frames);
                    split_path(bt_paths[1].c_str(), ":", want_bts);
                    if (want_frames.size() != want_bts.size()) continue;

                    for(size_t i = 0; i < want_frames.size(); i++) {
                        if (!is_positive_digit_string(want_frames[i])) {
                            matched = NULL;
                            break;
                        }
                        size_t idx = atoi(want_frames[i].c_str());
                        if (idx > frames - 1) {
                            matched = NULL;
                            break;
                        }
                        if (strstr(tp.getBacktraces().itemAt(idx).string(), want_bts[i].c_str())) {
                            matched = rule;
                        } else {
                            matched = NULL;
                            break;
                        }
                    }
                } else {
                    if (frames == 0) continue;

                    size_t begin = 0;
                    size_t end = frames - 1;

                    char* p_bt = parse_bt_line(rule->p_bt, &begin, &end);

                    for (size_t i = begin; i <= end; i++) {
                        if (strstr(tp.getBacktraces().itemAt(i).string(), p_bt)) {
                            matched = rule;
                            break;
                        }
                    }
                }
            } else if (rule->p_pn > RULE_UID_MAX) {
                matched = rule;
            }
#ifdef NDBG_LOCAL
            else {
                matched = rule;
            }
#endif
        }
    }

    if (matched) {
        MILOGE("matched rule: %d: %s", matched->i_id, matched->p_bt);
        RULE_FILE_T* p_file = matched->t_di.p_file;
        while (p_file) {
             if (RULE_FILE_TYPE_KEYWORD == p_file->c_type) {
                for (size_t i = 0; i < frames; i++) {
                    const char* path = strchr(tp.getBacktraces().itemAt(i).string(),'/');

                    if (path && strstr(path,p_file->p_str) // path contains keyword
                            && strncmp(matched->t_di.p_file->p_str,path,strlen(matched->t_di.p_file->p_str))) { // ignore repeated continous path
                        RULE_FILE_T* n_file = (RULE_FILE_T*)malloc(sizeof(RULE_FILE_T));

                        n_file->c_type = RULE_FILE_TYPE_PATH;
                        n_file->c_action = p_file->c_action;
                        n_file->next = matched->t_di.p_file;

                        const char* strend = strchr(path, ' ');
                        if (strend) {
                            size_t len  = strend - path;

                            if (len > PATH_MAX - 1) {
                                len = PATH_MAX - 1;
                            }
                            n_file->p_str = (char *)malloc(len + 1);
                            memcpy(n_file->p_str, path, len);
                            *(n_file->p_str + len) = 0;
                        } else {
                            n_file->p_str = strdup(path);
                        }

                        matched->t_di.p_file = n_file;
                    }
                }
            }
            p_file = p_file->next;
        }
    }
    return matched;
}

static void do_cmd(char* cmd,bool is_sync)
{
    MILOGD("do_cmd %s",cmd);

    pid_t pid = fork();
    if (!pid) {
        char *argp[4] = {(char*)"sh", (char*)"-c", NULL,NULL};

        argp[2] = (char *)cmd;
        int ret = execv(_PATH_BSHELL,argp);
        MILOGE("ret=%d,err=%d",ret,errno);
        _exit(-1);
    } else if (is_sync) {
        struct stat sb;
        char proc[PATH_MAX];
        int us = 100000;

        snprintf(proc, sizeof(proc),"/proc/%d",pid);

        while (us < 2000000 && !stat(proc, &sb)) {
           usleep(us);
           MILOGD("do_cmd cmd = %s sleep ns = %d", cmd, us);
           us = us*2;
        }
    }
}
#ifdef NDBG_LOCAL
#define MIUI_POST_PROC "miuindbg_post_local"
#else
#define MIUI_POST_PROC "miuindbg_post"
#endif

static void call_post_process(char* dir)
{
    char cmd[PATH_MAX];

    snprintf(cmd, sizeof(cmd), MIUI_POST_PROC " %s", dir);
    do_cmd(cmd, false);
}


static void dump_logs(char* dir, RULE_T* rule)
{
    char cmd[PATH_MAX];

    if (rule->t_di.mlog) {
        snprintf(cmd,sizeof(cmd),"logcat -v threadtime -b main -d > %s/main.log",dir);
        do_cmd(cmd, false);
    }

    if (rule->t_di.slog) {
        snprintf(cmd,sizeof(cmd),"logcat -v threadtime -b system -d > %s/system.log",dir);
        do_cmd(cmd, false);
    }

    if (rule->t_di.elog) {
        snprintf(cmd,sizeof(cmd),"logcat -v threadtime -b events -d > %s/events.log",dir);
        do_cmd(cmd, false);
    }

    if (rule->t_di.rlog) {
        snprintf(cmd,sizeof(cmd),"logcat -v threadtime -b radio -d > %s/radio.log",dir);
        do_cmd(cmd, false);
    }

    if (rule->t_di.klog) {
        snprintf(cmd,sizeof(cmd),"dmesg > %s/kernel.log",dir);
        do_cmd(cmd, false);
    }
}

static void dump_ps(char* dir, RULE_T* rule)
{
    if (!rule->t_di.ps) return;

    char cmd[PATH_MAX];

    snprintf(cmd,sizeof(cmd),"ps -t > %s/ps.log",dir);
    do_cmd(cmd, true);
}

static void dump_lsof(pid_t pid, char* dir, RULE_T* rule)
{
    if (!rule->t_di.lsof) return;

    char cmd[PATH_MAX];
    snprintf(cmd,sizeof(cmd),"lsof -p %d > %s/lsof.log",pid,dir);
    do_cmd(cmd, true);
}

static void dump_spec(char* dir, RULE_T* rule)
{
    if (!rule->t_di.spec) return;
    /*do special */

    MILOGI("dump_spec dir=%s,s_tombstone_path=%s",dir,s_tombstone_path);

    if (s_tombstone_path == NULL)
        return;
    FILE *fp = fopen(s_tombstone_path, "r");

    if (fp != NULL) {
        char line[1024];
        int count = 10;
        while (count-- && fgets(line, sizeof(line), fp)) {
            if (!strncmp(line, "--->", 4)) {
                char cmd[PATH_MAX];
                char* file = strchr(line,'/');
                if (file) {
                    MILOGI("dump_spec dump file = %s",file);
                    snprintf(cmd, sizeof(cmd), "ln -s %s %s%s", file, dir, strrchr(file,'/'));
                    do_cmd(cmd, false);
                }
            }
            break;
        }
        fclose(fp);
    }

    free(s_tombstone_path);
    s_tombstone_path = NULL;
}

static void dump_maps(char* dir, pid_t pid, RULE_T* rule)
{
    if (!rule->t_di.maps) return;

    char cmd[PATH_MAX];

    snprintf(cmd,sizeof(cmd),"cat /proc/%d/maps > %s/%d-maps.log",pid,dir,pid);
    do_cmd(cmd, true);
}

static void dump_file(char* dir, char* path)
{
    struct stat sb;

    if (!stat(path, &sb)) {
        char cmd[PATH_MAX];

        snprintf(cmd, sizeof(cmd), "ln -s %s %s%s", path, dir, strrchr(path,'/'));
        do_cmd(cmd, false);

    } else {
        MILOGE("cannot find file %s",path);
    }
}

static void dump_file_stat(char* dir, char* path)
{
    char cmd[PATH_MAX];

    snprintf(cmd, sizeof(cmd), "ls -l %s >> %s/stat.log", path, dir);
    do_cmd(cmd, false);
}

static char* find_apk_path(char* pkname)
{
    if (!pkname) return NULL;

    XMLDocument doc;
    XMLElement* root;

    doc.LoadFile("/data/system/packages.xml");
    root = doc.RootElement();

    if (!root) return NULL;

    const char* pkdir = NULL;

    if (strcmp(root->Name(),"packages")) return NULL;

    XMLElement* elem = root->FirstChildElement();
    while (elem && pkdir == NULL) {
        if (!strcmp(elem->Name(),"package")) {
            const char* p_attr = elem->Attribute("name", NULL);
            if (!strncmp(p_attr,pkname,strlen(p_attr))) {
                pkdir = elem->Attribute("codePath", NULL);
                break;
            }
        }
        elem = elem->NextSiblingElement();
    }

    if (!pkdir) return NULL;

    char* apkpath = NULL;
    char path[PATH_MAX];
    DIR *d;
    struct dirent *de;

    d = opendir(pkdir);
    if (!d) return NULL;

    while ((de = readdir(d))) {
        if (de->d_type == DT_DIR) continue;
        if (strstr(de->d_name,".apk")) {
            sprintf(path,"%s/%s",pkdir,de->d_name);
            apkpath = strdup(path);
            break;
        }
    }
    closedir(d);

    return apkpath;
}

static char *nexttok(char **strp)
{
    char *p = strsep(strp," ");
    return (p == 0) ? NULL : p;
}

static pid_t get_ppid(pid_t pid)
{
    char statline[1024];
    char *ptr = statline;

    sprintf(statline, "/proc/%d/stat", pid);
    int fd = open(statline, O_RDONLY);
    if (fd <= 0) return -1;

    int r = read(fd, statline, 1023);
    close(fd);

    if (r < 0) return -1;

    statline[r] = 0;

    nexttok(&ptr);              // skip pid
    ptr++;                      // skip "("

    ptr = strrchr(ptr, ')');    // Skip to *last* occurence of ')',
    ptr++;                      // skip )
    ptr++;                      // skip " "
    nexttok(&ptr);

    return atoi(nexttok(&ptr));
}

static void dump_package(char* dir, char* key, pid_t pid) {
    if (*key++ != '#') return; // first char must be ‘#’

    char* pkname = NULL;
    char pknamebuf[64];

    if (*key == '#') {
        char cmdline[64];

        while (pid > 1) {
            get_proc_name(pid,cmdline,sizeof(cmdline));

            MILOGI("dump_package proc pid=%d, name=%s", pid, cmdline);

            if (!cmdline[0] || !strncmp(cmdline,"zygote",6)) break;

            if (cmdline[0] != '/') {
                strcpy(pknamebuf,cmdline);
                pkname = pknamebuf;
            }
            pid = get_ppid(pid);
        }
    } else {
        pkname = key;
   }

    if (!pkname) {
        MILOGE("dump_package Not Found Java Package! ");
        return;
    }

    char* path = find_apk_path(pkname);

    if (!path) {
        MILOGE("dump_package Not Found the path of %s",pkname);
        return;
    }
    dump_file(dir,path);

    free(path);
}

static void dump_files(pid_t pid, char* dir, RULE_T* rule)
{
    RULE_FILE_T* p_file = rule->t_di.p_file;
    while (p_file) {
        if (RULE_FILE_TYPE_PATH == p_file->c_type) {
            if (RULE_FILE_ACTION_DUMP == p_file->c_action) {
                if (p_file->p_str[0] == '#') {
                    dump_package(dir,p_file->p_str,pid);
                } else {
                    dump_file_stat(dir, p_file->p_str);
                    dump_file(dir, p_file->p_str);
                }
            } else if (RULE_FILE_ACTION_STAT == p_file->c_action) {
                dump_file_stat(dir, p_file->p_str);
            }
        }
        p_file = p_file->next;
    }
}
#define JAVA_STACK_LIBRARY          "libjavastackimpl.so"
#define DUMP_CURRENT_STACK          "dump_current_thread"
#define DUMP_ALL_STACK              "dump_all_threads"


extern "C" int ptrace_call(pid_t, pid_t, const char*, const char*, uintptr_t*, uintptr_t, uintptr_t* );

static void dump_javastack(pid_t pid, pid_t tid, RULE_T* rule)
{
    if (!rule->t_di.jstk) return;

    const char* impl_func_str = DUMP_CURRENT_STACK;

    if (RULE_JSTK_TYPE_ALL == rule->t_di.jstk) {
        impl_func_str = DUMP_ALL_STACK;
    }
    ptrace_call(pid, tid, JAVA_STACK_LIBRARY, impl_func_str, NULL, 0, NULL);
}

#define TOMBSTONE_DIR       "/data/tombstones"
#define TOMBSTONE_PREFIX    "tombstone_"

static bool match_line_and_copy_to_dir(char* file, char* dir, char* str)
{
    bool ret = false;
    int head_len = 4;
    int match_len = strlen(str);

    if (match_len < head_len) {
        return ret;
    }

    FILE *fp = fopen(file, "r");

    if (fp != NULL) {
        char line[1024];
        int count = 10;
        while (count-- && fgets(line, sizeof(line), fp)) {
            if (!strncmp(line, str, head_len)) {
                if (!strncmp(line + head_len, str + head_len, match_len - head_len)) {
                    char cmd[PATH_MAX];

                    snprintf(cmd, sizeof(cmd), "ln -s %s %s%s", file, dir, strrchr(file,'/'));
                    do_cmd(cmd, false);
                    if (s_tombstone_path)
                        free(s_tombstone_path);
                    s_tombstone_path = strdup(file);
                    ret = true;
                }
                break;
            }
        }
        fclose(fp);
    }
    return ret;
}

static void dump_tombstone(pid_t pid, pid_t tid, char* dir, RULE_T* rule)
{
    if (!rule->t_di.tomb) return;

    DIR *d;
    char path[PATH_MAX];
    struct dirent *de;
    char ti[64];

    snprintf(ti,sizeof(ti),"pid: %d, tid: %d, name:",pid,tid);

    d = opendir(TOMBSTONE_DIR);
    if (!d) return;

    long latest = 0;
    char* tomb = NULL;
    while ((de = readdir(d))) {
        if (de->d_type == DT_DIR || strncmp(de->d_name, TOMBSTONE_PREFIX, sizeof(TOMBSTONE_PREFIX)-1)) {
            continue;
        }

        struct stat sb;
        snprintf(path, sizeof(path), TOMBSTONE_DIR"/%s", de->d_name);
        if (!stat(path, &sb)) {
            if ((long)sb.st_mtime > latest) {
                latest = sb.st_mtime;
                if (tomb) {
                    free(tomb);
                }
                tomb = strdup(de->d_name);
            }
        }
    }
    closedir(d);

    if (tomb) {
        snprintf(path, sizeof(path), TOMBSTONE_DIR"/%s", tomb);
        free(tomb);

        if (match_line_and_copy_to_dir(path, dir, ti)) return;
    }

    d = opendir(TOMBSTONE_DIR);
    if (!d) return;

    while ((de = readdir(d))) {
        if (de->d_type == DT_DIR || strncmp(de->d_name, TOMBSTONE_PREFIX, sizeof(TOMBSTONE_PREFIX)-1)) continue;

        snprintf(path, sizeof(path), TOMBSTONE_DIR"/%s", de->d_name);

        if (match_line_and_copy_to_dir(path, dir, ti)) break;
    }
    closedir(d);
}

#define CORE_PATTERN_NODE       "/proc/sys/kernel/core_pattern"
#define CORE_PATTERN            MIUI_NATIVE_DEBUG_ROOT_DIR"/core/core-%e-%p"

struct rlimit64_t {
    __u64 rlim_cur;
    __u64 rlim_max;
};

static int core_get_pattern(char* buf,int len)
{
    int fd = open(CORE_PATTERN_NODE, O_RDONLY);

    if (fd > 0) {
        int read_len = read(fd,buf,len-1);
        close(fd);
        return read_len;
    }
    return -1;
}

static void core_set_dumpable(pid_t pid, pid_t tid)
{
    uintptr_t params[] = {PR_SET_DUMPABLE,1,0,0,0};

    ptrace_call(pid,tid,"libc.so","prctl",params,sizeof(params)/sizeof(uintptr_t),NULL);
}

#define COREDUMP_PROP "sys.miui.ndcd"

static void core_set_pattern()
{
#if NDBG_LOCAL
    int fd = open(CORE_PATTERN_NODE, O_WRONLY);
    if (fd > 0) {
        write(fd, CORE_PATTERN, sizeof(CORE_PATTERN));
        close(fd);
    }
    if (access(MIUI_NATIVE_DEBUG_CORE_DIR,F_OK)) {
        if (mkdir(MIUI_NATIVE_DEBUG_CORE_DIR, S_IRWXU | S_IXGRP | S_IXOTH))
            return;
        chmod(MIUI_NATIVE_DEBUG_CORE_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
        chown(MIUI_NATIVE_DEBUG_CORE_DIR, AID_SYSTEM, AID_SYSTEM);
    }
#else
    char path[PATH_MAX];
    char len = sizeof(CORE_PATTERN) -1;
    int retry = 10;
    int us = 5*1000;

    property_set(COREDUMP_PROP, "on");

    while (retry--) {
        int pat_len = core_get_pattern(path,sizeof(path));

        if (len <= pat_len && !strncmp(path,CORE_PATTERN,len)) break;

        usleep(us);
        us *=2;
    }
#endif
}

static void core_set_filter(pid_t pid, bool is_full)
{
    char path[PATH_MAX];

    snprintf(path,sizeof(path),"/proc/%d/coredump_filter",pid);

    int fd = open(path, O_WRONLY);

    if (fd > 0) {
        if (is_full) {
            write(fd, "39", 2);    /*0x27 MMF_DUMP_ANON_PRIVATE|MMF_DUMP_ANON_SHARED|MMF_DUMP_MAPPED_PRIVATE|MMF_DUMP_HUGETLB_PRIVATE*/
        } else {
            write(fd, "35", 2);    /*0x23 MMF_DUMP_ANON_PRIVATE|MMF_DUMP_ANON_SHARED|MMF_DUMP_HUGETLB_PRIVATE*/
        }
        close(fd);
    }
}

static void core_set_limit(pid_t pid,unsigned long size)
{
    if (!pid || pid == getpid()) {
        struct rlimit rlim;

        rlim.rlim_cur = size;
        rlim.rlim_max = size;
        setrlimit(RLIMIT_CORE, &rlim);
    } else {
        struct rlimit64_t rlim64;

        rlim64.rlim_cur = size;
        rlim64.rlim_max = size;
        syscall(__NR_prlimit64, pid, RLIMIT_CORE, &rlim64, NULL);
    }
}
static void trim_core_dump(pid_t pid, pid_t tid , bool dump_java) {
    uintptr_t param = 0;
    if (dump_java)
        param = 1;

    ptrace_call(pid, tid, JAVA_STACK_LIBRARY, "trim_core_dump", &param, 1, NULL);
}

static void dump_core(pid_t pid, pid_t tid, RULE_T* rule)
{
    if (!rule->t_di.core) return;

    core_set_dumpable(pid,tid);
    core_set_pattern();
    if (RULE_CORE_TYPE_FULL == rule->t_di.core) {
        core_set_filter(pid, true);
        trim_core_dump(pid, tid, true);
    } else {
        core_set_filter(pid, false);
        trim_core_dump(pid, tid, false);
    }
    core_set_limit(pid,s_core_limit);
}

static bool make_dir(unsigned int id, char* buf)
{
    sprintf(buf, MIUI_NATIVE_DEBUG_ROOT_DIR"/%08d_", id);

    time_t tm = time(NULL);
    struct tm _tm;
    localtime_r(&tm, &_tm);
    strftime(buf+strlen(buf), sizeof("20160125232233_"), "%Y%m%d%H%M%S_", &_tm);

    char ver[PROPERTY_VALUE_MAX];
    property_get("ro.build.version.incremental", ver, "unknown");

    strcat(buf,ver);

    if (access(buf,F_OK)) {
        if (mkdir(buf, S_IRWXU | S_IXGRP | S_IXOTH)) return false;

        chmod(buf, S_IRWXU | S_IRWXG | S_IRWXO);
    }
    return true;
}

static void dump_info(pid_t pid, pid_t tid, RULE_T* rule)
{
    char dir[PATH_MAX];

    if (!rule || !make_dir(rule->i_id,dir)) return;

    dump_javastack(pid,tid,rule);
    dump_files(pid,dir,rule);
    dump_ps(dir,rule);
    dump_tombstone(pid,tid,dir,rule);
    dump_lsof(pid,dir,rule);
    dump_spec(dir,rule);
    dump_maps(dir,pid,rule);
    dump_core(pid,tid,rule);

    dump_logs(dir,rule);

    call_post_process(dir);
}

static void change_dir(const char* filename, const char* old_dir, const char* new_dir)
{
    char old_path[PATH_MAX];
    char new_path[PATH_MAX];

    snprintf(old_path,sizeof(old_path), "%s/%s",old_dir,filename);
    snprintf(new_path,sizeof(new_path), "%s/%s",new_dir,filename);
    rename(old_path,new_path);
}

static bool check_post_process()
{
    DIR *d;
    struct dirent *de;

    d = opendir("/proc");
    if (d == 0) return false;

    int ret = false;

    while ((de = readdir(d)) != 0){
        if (isdigit(de->d_name[0])){
            char cmdline[64];
            pid_t pid = atoi(de->d_name);

            get_proc_name(pid,cmdline,sizeof(cmdline));

            if (!strcmp(cmdline,"/system/bin/" MIUI_POST_PROC)) {
                ret = true;
                break;
            }

        }
    }
    closedir(d);
    return ret;
}

static void clean_dir(const char* dir)
{
    DIR *d;
    struct dirent *de;
    char path[PATH_MAX];

    strcpy(path,dir);
    d = opendir(dir);
    if (d) {
        bool isRootDir = !strcmp(dir,MIUI_NATIVE_DEBUG_RULES_XML);

        while ((de = readdir(d)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")
#ifdef NDBG_LOCAL
                || (isRootDir && !strcmp(de->d_name, "rules_local.xml")))
#else
                || (isRootDir && !strcmp(de->d_name, "rules.xml")))
#endif
                continue;

            snprintf(path,sizeof(path),"%s/%s", dir, de->d_name);
            if (de->d_type == DT_DIR) {
                clean_dir(path);
            }
            remove(path);
        }
        closedir(d);
    }
}

static void check_dir()
{
    struct stat st;

    if (!stat(MIUI_NATIVE_DEBUG_RULES_XML,&st)) {
        if (s_rules_mtime != (long)st.st_mtime){
            s_rules_mtime = (long)st.st_mtime;
            s_dump_enable = true;
            distroy_rules();
        }
    } else {
        s_dump_enable = false;
    }

    if (!access(MIUI_NATIVE_DEBUG_LOCK,F_OK)) {
        if (check_post_process()) {
            s_dump_enable = false;
        } else {
            clean_dir(MIUI_NATIVE_DEBUG_ROOT_DIR);
        }
    }

    if (!s_dump_enable) return;

#ifndef NDBG_LOCAL
    DIR *d,*subd;
    struct dirent *de,*subde;

    d = opendir(MIUI_NATIVE_DEBUG_ZIP_DIR);
    if (d) {
        while ((de = readdir(d))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;

            if (strstr(de->d_name,".miz") || strstr(de->d_name,".zip")) {
                s_dump_enable = false;
                break;
            }
        }
        closedir(d);
    }

    if (!s_dump_enable) return;

    d = opendir(MIUI_NATIVE_DEBUG_CORE_DIR);
    if (d) {
        char path[PATH_MAX];

        while ((de = readdir(d))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;

            snprintf(path,sizeof(path), MIUI_NATIVE_DEBUG_CORE_DIR"/%s",de->d_name);

            if (s_dump_enable && !strncmp(de->d_name,"core-debuggerd",14)) {
                char dir[PATH_MAX];

                if (make_dir(0,dir)) {
                    change_dir(de->d_name,MIUI_NATIVE_DEBUG_CORE_DIR,dir);
                    call_post_process(dir);
                    s_dump_enable = false;
                    continue;
                }
            }
            remove(path);
        }
        closedir(d);
    }
#endif

    struct statfs stfs;

    if (statfs(MIUI_NATIVE_DEBUG_ROOT_DIR, &stfs) >= 0) {
        unsigned long long freeSize = (unsigned long long)(stfs.f_bfree*stfs.f_bsize);

        if (freeSize < MIUI_NATIVE_FREE_SPACE_MIN) {
            MILOGI("check_dir check size false");
            s_core_limit = 0;
            s_dump_enable = false;
        } else {
            if (freeSize > MIUI_NATIVE_FREE_SPACE_MAX)
                freeSize = MIUI_NATIVE_FREE_SPACE_MAX;

            s_core_limit = (freeSize/4*3)/0x10000*0x10000;
        }
    }
}

extern "C" void miui_native_debug_process(pid_t pid, pid_t tid)
{
    check_dir();

    if (s_dump_enable) {

        build_rules();

        RULE_T* rule = match_rules(pid,tid);

        if (rule) {
            dump_info(pid, tid, rule);
#ifndef NDBG_LOCAL
            distroy_rules();
            s_dump_enable = false;
#endif
        }
    } else {
        distroy_rules();
    }
}
