#include "../../../common/native/include/util_common.h"
#include "../../../common/native/include/test_common.h"

#include <binder/IServiceManager.h>
#include <IPriviledgedProcService.h>
#include <utils/Mutex.h>

#include <stdio.h>

static const android::sp<IPriviledgedProcService> & getPriviledgedProcService() {
    static android::sp<IPriviledgedProcService> sService(NULL);
    static android::Mutex sLock;
    android::Mutex::Autolock autolock(sLock);

    bool isBinderAlive = false;
    if (sService != NULL) {
        TLOGI("getPriviledgedProcService: sService != NULL. ");
        isBinderAlive =
#ifdef BINDER_STATIC_AS_BINDER
            android::IInterface::asBinder(sService)->pingBinder() == android::NO_ERROR;
#else
            sService->asBinder()->pingBinder() == android::NO_ERROR;
#endif
    } else {
        TLOGI("getPriviledgedProcService: sService == NULL. ");
    }

    if (!isBinderAlive) {
       TLOGW("getPriviledgedProcService: binder not alive. ");
       android::sp<android::IServiceManager> sm = android::defaultServiceManager();
       do {
          android::sp<android::IBinder> binder = sm->getService(android::String16("miui.fdpp"));
          if (binder != NULL) {
            sService = android::interface_cast<IPriviledgedProcService>(binder);
            break;
          }
          TLOGE("Failed to get PriviledgedProcService binder. ");
          sleep(1);
       } while (true);
    } else {
        TLOGI("getPriviledgedProcService: binder alive. ");
    }

    return sService;
}

// ROOT && REMOUNT assumed.
void doTest() {

#define PERSISTENT_DIR "/persist"

    android::sp<IPriviledgedProcService> service1, service2;
    TEST_ACTION(service1 = getPriviledgedProcService();)
    TEST_ACTION(service2 = getPriviledgedProcService();)
    TEST_ASSERT_NOT_EQUAL_PLAIN((void *)NULL, service1.get());
    TEST_ASSERT_NOT_EQUAL_PLAIN((void *)NULL, service2.get());
    TEST_ASSERT_EQUAL_PLAIN(service1, service2);

    bool ex = false;
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->exists("/data", &ex));
    TEST_ASSERT_EQUAL_PLAIN(true, ex);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->exists("/data/system/users/0.xml", &ex));
    TEST_ASSERT_EQUAL_PLAIN(true, ex);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->exists("/data/notext", &ex));
    TEST_ASSERT_EQUAL_PLAIN(false, ex);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->exists("/data/system/users/0.xml/abc", &ex));
    TEST_ASSERT_EQUAL_PLAIN(false, ex);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->exists("/init.rc", &ex));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->exists("/init.rc/notext", &ex));
    TEST_ASSERT_EQUAL_PLAIN(false, ex);


    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->remove("/data/notext_test"));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ACTION(system("touch " PERSISTENT_DIR "/ext_test");)
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->remove(PERSISTENT_DIR "/ext_test"));
    TEST_ACTION(system("touch " "/data/ext_test");)
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->remove("/data/ext_test"));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);

    std::vector<std::vector<char> > list;
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->lsDir("/data", &list));
    const char * s_data = "data";
    std::vector<char> v_data(s_data, s_data + ::strlen(s_data) + 1);
    TEST_ASSERT_EQUAL_PLAIN(true, std::find(list.begin(), list.end(), v_data) != list.end());
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->lsDir("/data/noex", &list));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->lsDir("/mnt", &list));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);

    bool dir = false;
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->isDir("/data", &dir));
    TEST_ASSERT_EQUAL_PLAIN(true, dir);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->isDir("/data/system/users/0.xml", &dir));
    TEST_ASSERT_EQUAL_PLAIN(false, dir);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isDir("/data/noext", &dir));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isDir("/init.rc", &dir));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isDir("/init.rc/noext", &dir));
    TEST_ASSERT_EQUAL_PLAIN(ENOTDIR, errno);

    bool file = false;
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->isFile("/data", &file));
    TEST_ASSERT_EQUAL_PLAIN(false, file);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->isFile("/data/system/users/0.xml", &file));
    TEST_ASSERT_EQUAL_PLAIN(true, file);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isFile("/data/noext", &file));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isFile("/init.rc", &file));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isFile("/init.rc/noext", &file));
    TEST_ASSERT_EQUAL_PLAIN(ENOTDIR, errno);

    TEST_ACTION(system("touch " PERSISTENT_DIR "/move_test");)
    TEST_ACTION(getPriviledgedProcService()->remove(PERSISTENT_DIR "/move_test2");)
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isFile(PERSISTENT_DIR "/move_test2", &file));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->move(PERSISTENT_DIR "/move_test", PERSISTENT_DIR "/move_test2"));
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isFile(PERSISTENT_DIR "/move_test", &file));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->isFile(PERSISTENT_DIR "/move_test2", &file));
    TEST_ASSERT_EQUAL_PLAIN(true, file);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->move(PERSISTENT_DIR "/move_test", PERSISTENT_DIR "/move_test2"));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ACTION(system("touch " "/data/move_test");)
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->move("/data/move_test", "/data/move_test2"));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);

    TEST_ACTION(getPriviledgedProcService()->remove(PERSISTENT_DIR "/mkdir_test");)
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->isDir(PERSISTENT_DIR "/mkdir_test", &dir));
    TEST_ASSERT_EQUAL_PLAIN(ENOENT, errno);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->mkdir(PERSISTENT_DIR "/mkdir_test"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->isDir(PERSISTENT_DIR "/mkdir_test", &dir));
    TEST_ASSERT_EQUAL_PLAIN(true, dir);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->mkdir(PERSISTENT_DIR "/mkdir_test"));
    TEST_ASSERT_EQUAL_PLAIN(EEXIST, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->mkdir("/system/bin/mkdir_test"));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);

    const char * s_tdata1 = "ABCDEFG\n";
    std::vector<unsigned char> v_tdata1(s_tdata1, s_tdata1 + strlen(s_tdata1));
    const char * s_tdata2 = "123456\n";
    std::vector<unsigned char> v_tdata2(s_tdata2, s_tdata2 + strlen(s_tdata2));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->writeTinyFile(
                                      PERSISTENT_DIR "/write_test",
                                      false,
                                      addr(v_tdata1),
                                      v_tdata1.size()));
    std::vector<unsigned char> bytes;
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->readTinyFile(PERSISTENT_DIR "/write_test", &bytes));
    TEST_ASSERT_EQUAL_PLAIN(v_tdata1, bytes);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->writeTinyFile(
                                      PERSISTENT_DIR "/write_test",
                                      false,
                                      addr(v_tdata2),
                                      v_tdata2.size()));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->readTinyFile(PERSISTENT_DIR "/write_test", &bytes));
    TEST_ASSERT_EQUAL_PLAIN(v_tdata2, bytes);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->writeTinyFile(
                                          PERSISTENT_DIR "/write_test",
                                          true,
                                          addr(v_tdata1),
                                          v_tdata1.size()));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->readTinyFile(PERSISTENT_DIR "/write_test", &bytes));
    std::vector<unsigned char> v_combined(v_tdata2);
    TEST_ACTION(v_combined.insert(v_combined.end(), v_tdata1.begin(), v_tdata1.end());)
    TEST_ASSERT_EQUAL_PLAIN(v_combined, bytes);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->writeTinyFile(
                                          "/system/bin/write_test",
                                          true,
                                          addr(v_tdata1),
                                          v_tdata1.size()));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);
    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->readTinyFile("/init.rc", &bytes));
    TEST_ASSERT_EQUAL_PLAIN(EACCES, errno);

    TEST_ASSERT_EQUAL_PLAIN(false, getPriviledgedProcService()->addService("NO_ACCESS",
                            android::defaultServiceManager()->getService(android::String16("miui.fdpp"))));
    TEST_ASSERT_EQUAL_PLAIN(EPERM, errno);
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->addService("miui.fdpp",
                            android::defaultServiceManager()->getService(android::String16("miui.fdpp"))));

    std::vector<char> oldValue;
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, true, NULL));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, oldValue.size());
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, true, ""));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(1, oldValue.size());
    TEST_ASSERT_EQUAL_PLAIN(0, ::strcmp(addr(oldValue), ""));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, true, "TEST"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, ::strcmp(addr(oldValue), "TEST"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, "BAD"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, ::strcmp(addr(oldValue), "TEST"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, NULL));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, ::strcmp(addr(oldValue), "TEST"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, true, NULL));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN((size_t)0, oldValue.size());
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, true, NULL));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, oldValue.size());
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test2",  &oldValue, true, NULL));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test2",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, oldValue.size());
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, true, "v.test.test"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test2",  &oldValue, true, "v.test.test.2"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, ::strcmp(addr(oldValue), "v.test.test"));
    TEST_ASSERT_EQUAL_PLAIN(true, getPriviledgedProcService()->getSetRebootClearVariable("test.test2",  &oldValue, false, ""));
    TEST_ASSERT_EQUAL_PLAIN(0, ::strcmp(addr(oldValue), "v.test.test.2"));
}

int main() {

    BEGIN_TEST;
    doTest();
    END_TEST;

    return 0;
}


