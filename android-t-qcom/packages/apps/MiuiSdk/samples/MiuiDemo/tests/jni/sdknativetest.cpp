
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/Log.h>
#include "utils.h"

#include <string.h>
#include <string>
#include <vector>
#include <utility>

#include "ExecMemAlloctor.h"

using std::string;
using std::vector;
using std::pair;

using miui::art::ExecMemAlloctor;
using miui::art::ExecMemAccess;

static const char* format(const char* fmt, ...) {
    va_list arg;
    static char sz[1024];
    va_start(arg, fmt);
    vsprintf(sz, fmt, arg);
    va_end(arg);
    return sz;
}

struct mapinfo {
    void *start;
    void *end;
    string info;

    mapinfo(void *start, void*end, const char* info)
    :start(start), end(end), info(info) { }
    mapinfo(const mapinfo& mi)
    :start(mi.start), end(mi.end), info(mi.info){ }
};

typedef vector<mapinfo> MapInfoArray;

//readmmaps
struct MMapsFile {
    FILE *fp;
    char szFilter[128];
    char szCurLine[1024];

    MMapsFile(const char* filter) {
        if (filter != NULL) {
            strcpy(szFilter, filter);
        } else {
            szFilter[0] = '\0';
        }

        char szFile[256];
        sprintf(szFile, "/proc/%d/maps", getpid());
        fp = fopen(szFile, "rt");
        szCurLine[0] = '\0';
    }

    const char* curLine() { return feof(fp) ? NULL : szCurLine; }

    const char* getNextLine() {
        while(getLine() && !filter());
        return curLine();
    }

    bool iseof() { return feof(fp); }

    ~MMapsFile() {
        fclose(fp);
    }

private:
    bool getLine() {
        int i = 0;
        while (!feof(fp)) {
            szCurLine[i] = fgetc(fp);
            if (szCurLine[i] == '\n') {
                szCurLine[i] = '\0';
                return true;
            }
            i ++;
        }
        return NULL;
    }

    bool filter() {
        if (szFilter[0] == 0) return true;
        if (strstr(szCurLine, szFilter) != 0) return true;
        return false;
    }
};

bool readMiuiClassproxyFromMaps(MapInfoArray &maps) {
    MMapsFile mf("miui-classproxy");

    while(!mf.iseof()) {
        const char* info = mf.getNextLine();
        if (info == NULL) break;
        unsigned long p1, p2;
        sscanf(info, "%lx-%lx r", &p1, &p2);
        maps.push_back(mapinfo(reinterpret_cast<void*>(p1), reinterpret_cast<void*>(p2), info));
    }
    return true;
}


template<const unsigned int UNIT = 8>
struct ExecuteMemoryTest {
    JNIEnv *env;
    ExecMemAlloctor<UNIT> alloctor;

    ExecuteMemoryTest(JNIEnv *env) : env(env) { }

    void testMassAllocFree(int count) {
        void **ptrs = new void*[count];

        MapInfoArray startmaps;
        readMiuiClassproxyFromMaps(startmaps);

        srand(time(NULL));
        for (int i = 0; i < count; i++) {
            size_t size = rand() % 1024;
            ptrs[i] = alloctor.alloc(size);

            ExecMemAccess access(ptrs[i], size);
            JavaAssertTrue(env, format("UNIT=%d, COUNT=%d: Assert Alloc & Access %p (size:%d) canwrite (error:%s)", UNIT, count, ptrs[i], size, strerror(errno)), access.canWrite());
        }

        MapInfoArray maps;
        readMiuiClassproxyFromMaps(maps);
        //check pointer in maps
        for (int i = 0; i < count; i ++) {
            bool find = false;
            for(MapInfoArray::iterator it = maps.begin(); it != maps.end(); ++it) {
                mapinfo& p = *it;

                if (p.start <= ptrs[i] && ptrs[i] < p.end) {
                    find = true;
                    break;
                }
            }
            JavaAssertTrue(env, format("UNIT=%d, COUNT=%d: Assert Alloc In maps: %p", UNIT, count, ptrs[i]), find);
        }

        //clear
        for (int i = 0; i < count; i++) {
            alloctor.free(ptrs[i]);
        }

        maps.clear();
        readMiuiClassproxyFromMaps(maps);
        JavaAssertEquals(env, format("UNIT=%d, COUNT=%d: Assert miui classproxy maps cleared", UNIT, count), startmaps.size(), maps.size());

        delete[] ptrs;
    }


    void testInterleaveAllocFree(int count) {

        MapInfoArray startmaps;
        readMiuiClassproxyFromMaps(startmaps);

        srand(time(NULL));
        void **ptrs = new void*[count];
        memset(ptrs, 0, sizeof(void*)*count);

        for (int i = 0; i < count; i++) {
            size_t size = rand() % 1024;

            if (size % 4 == 0) {
                int j = rand() % count;
                if (ptrs[j] != NULL) {
                    alloctor.free(ptrs[j]);
                    ptrs[j] = NULL;
                    continue;
                }
            }

            ptrs[i] = alloctor.alloc(size);
            ExecMemAccess access(ptrs[i], size);
            JavaAssertTrue(env,
                format("UNIT=%d, COUNT=%d: Assert Alloc & Access Interleave %p (size:%d) canwrite (error:%s)", UNIT, count, ptrs[i], size, strerror(errno)),
                access.canWrite());
        }

        for (int i = 0; i < count; i++) {
            if (ptrs[i] != NULL) {
                alloctor.free(ptrs[i]);
            }
        }

        MapInfoArray maps;
        readMiuiClassproxyFromMaps(maps);
        JavaAssertEquals(env, format("UNIT=%d, COUNT=%d: Assert miui classproxy cleared Interleave", UNIT, count), startmaps.size(), maps.size());

        delete[] ptrs;
    }

};

template<const unsigned int UNIT>
void testExecuteMemory (JNIEnv *env) {
    ExecuteMemoryTest<UNIT> test(env);

    test.testMassAllocFree(100);
    test.testInterleaveAllocFree(100);

    test.testMassAllocFree(200);
    test.testInterleaveAllocFree(200);
}



/*
 * Class:     com_miui_sdknative_SdkNativeTest
 * Method:    testExecuteMemory
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_miui_sdknative_SdkNativeTest_testExecuteMemory
  (JNIEnv *env, jobject obj) {
    JavaAssertInit(env);

    TRY {
        testExecuteMemory<8>(env);
        testExecuteMemory<16>(env);
        testExecuteMemory<32>(env);
        testExecuteMemory<48>(env);
        testExecuteMemory<56>(env);
    } DEFAULT_CATCH {
        //Ignore, return to java
        ALOGE("SdkNativeTest: Assert Failed!");
    }
    END_TRY
}



