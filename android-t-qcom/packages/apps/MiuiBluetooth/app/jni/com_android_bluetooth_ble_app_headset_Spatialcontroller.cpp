#include "com_android_bluetooth_ble_app_headset_Spatialcontroller.h"

#define DATA_INCOMING_TIMEOUT  12000
#define DATA_SEND_INTERVAL 30
#define ADDR_STR_LENGTH 14
#ifdef __cplusplus
extern "C" {
#endif


char m_addr[ADDR_STR_LENGTH];

static void destroy_hid_point();

spatial_ctrl_t m_spatial_ctrl{
.fd = -1,
.is_polling = false
};


void lib_spatial_init() {
    m_spatial_ctrl.fd = -1;
    m_spatial_ctrl.is_polling = false;
}

void get_dynamic_device_uuid(uint8_t* uuid , char addr[ADDR_STR_LENGTH]) {
    LOGE("%s",addr);
    uint8_t buf [16] = {0};
    long long lsb = LSB_PREFIX_BT;
    lsb |= strtol(addr, NULL, 16);
    uint8_t * src = (uint8_t *) &lsb;
    for(int i = 15, j = 0; i >= 8; i--, j++) {
        memcpy(&buf[i], &src[j], 1);
    }
    memcpy(uuid, buf, 16);
}

static int uhid_write(int fd, const struct uhid_event *ev) {
    ssize_t ret;
    ret = write(fd, ev, sizeof(*ev));
    if (ret < 0) {
      return -errno;
    } else if (ret != sizeof(*ev)) {
      return -EFAULT;
    } else {
      return 0;
    }
}

/* This function be used for timer to send spatialdata */
static int send_event(int fd,int16_t ret[6]) {
    struct uhid_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = UHID_INPUT;
    ev.u.input.size = 1 +  2 * 3 + 2 *3 + 1;
    char * p = (char*)ret;
    static int16_t  t = 0;
    ev.u.input.data[0] = 0x1;
    memcpy(ev.u.input.data + 1, ret,sizeof(int16_t) * 6);
    ev.u.input.data[ 1 + sizeof(int16_t) * 6] = 0;

    return uhid_write(fd, &ev);
}


static int event(int fd)
{
  struct uhid_event ev;
  ssize_t ret;
  memset(&ev, 0, sizeof(ev));
  ret = read(fd, &ev, sizeof(ev));
  if (ret == 0) {
    return -EFAULT;
  } else if (ret < 0) {
    return -errno;
  } else if (ret != sizeof(ev)) {

    return -EFAULT;
  }
      int t = 0;
    switch (ev.type) {
      case UHID_START:
        LOGE("UHID_START from uhid-dev\n");
        break;
      case UHID_STOP:
        LOGE("UHID_STOP from uhid-dev\n");
        break;
      case UHID_OPEN:
        LOGE("UHID_OPEN from uhid-dev\n");
        break;
      case UHID_CLOSE:
        LOGE("UHID_CLOSE from uhid-dev\n");
        break;

      case UHID_GET_REPORT:
      LOGE(
        "UHID_GET_REPORT: Report type = %d, report_size = %d"
        " report id = %d report num %d\n",
        ev.u.set_report.rtype, ev.u.set_report.size, ev.u.set_report.id, ev.u.set_report.rnum);
        for(t = 0; t < ev.u.set_report.size; t++) {
          fprintf(stderr, "UHID_SET_REPORT +%x", ev.u.set_report.data[t]);
        }
        switch (ev.u.get_report.rnum) {
          case 1:
              ev.type = UHID_GET_REPORT_REPLY;
              ev.u.get_report_reply.id = ev.u.get_report.id;
              ev.u.get_report_reply.err = 0;
              char ret[2];
              ret[0] = ev.u.get_report.id;
              ret[1] = (0x3f << 2) |  (0x1 << 1) | 0x1;
              ev.u.get_report_reply.size = 2;
              memcpy(ev.u.get_report_reply.data,ret, 2);
              uhid_write(fd, &ev);
           break;
           case 2:
                ev.type = UHID_GET_REPORT_REPLY;
                ev.u.get_report_reply.id = ev.u.get_report.id;
                ev.u.get_report_reply.err = 0;
                uint8_t buf [16] = {0};
                get_dynamic_device_uuid(buf,m_addr);
                ev.u.get_report_reply.data[0] = ev.u.get_report.id;
                memcpy(ev.u.get_report_reply.data + 1, "#AndroidHeadTracker#1.0", 23);
                memcpy(ev.u.get_report_reply.data + 1+ 23, buf , 16);
                ev.u.get_report_reply.size = 40;
                uhid_write(fd, &ev);
      break;
        }
      break;
      case UHID_SET_REPORT:
            LOGE("UHID_SET_REPORT: Report type = %d, report_size = %d"
            " report id = %d report num %d\n",
            ev.u.set_report.rtype, ev.u.set_report.size, ev.u.set_report.id, ev.u.set_report.rnum);
            ev.type =  UHID_SET_REPORT_REPLY;
            ev.u.set_report_reply.id = ev.u.set_report.id;
            ev.u.set_report_reply.err = 0;
            uhid_write(fd, &ev);

      break;
      default:
        LOGE("Invalid event from uhid-dev: %u\n", ev.type);
    }
    return 0;
}


static inline pthread_t create_thread(void* (*start_routine)(void*),
                                      void* arg) {
  pthread_attr_t thread_attr;

  pthread_attr_init(&thread_attr);
  pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
  pthread_t thread_id = -1;
  if (pthread_create(&thread_id, &thread_attr, start_routine, arg) != 0) {
    return -1;
  }
  return thread_id;
}


static void* virtual_device_event_thread(void* arg) {
    struct pollfd pfds[1];
    pfds[0].fd = m_spatial_ctrl.fd;
    pfds[0].events = POLLIN;
    int ret = -1;
    LOGE("virtual_device_event_thread create");
    while(m_spatial_ctrl.is_polling) {
    ret = poll(pfds, 1, -1);
    if(ret < 0) {
        m_spatial_ctrl.is_polling = false;
    }
    if (pfds[0].revents & POLLIN) {
      ret = event(pfds[0].fd);
      if (ret)
        break;
    }
    }
    LOGD("dynamic poll thread exit");
    destroy_hid_point();
    return 0;
}

static bool create_hid_point() {
    if(m_spatial_ctrl.fd >0)
        destroy_hid_point();
    const char *dev_path = "/dev/uhid";
    m_spatial_ctrl.fd = open(dev_path, O_RDWR | O_CLOEXEC);
    if (m_spatial_ctrl.fd < 0) {
        LOGD("open dev/uhid fail");
        return -1;
    }
    struct uhid_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = UHID_CREATE;
    strcpy((char*)ev.u.create.name, "MMA-BLUETOOTH-DEVICE");
    ev.u.create.rd_data = ReportDescriptor;
    ev.u.create.rd_size = sizeof(ReportDescriptor);
    ev.u.create.bus = BUS_USB;
    ev.u.create.vendor = 0x15d9;
    ev.u.create.product = 0x0a37;
    ev.u.create.version = 0;
    ev.u.create.country = 0;
    m_spatial_ctrl.is_polling = true;
    create_thread(virtual_device_event_thread, NULL);
    return uhid_write(m_spatial_ctrl.fd, &ev);
}


static void destroy_hid_point() {
    struct uhid_event ev;
    LOGD("hid point destroy");
    if(m_spatial_ctrl.fd > 0) {
      memset(&ev, 0, sizeof(ev));
      ev.type = UHID_DESTROY;
      uhid_write(m_spatial_ctrl.fd, &ev);
      m_spatial_ctrl.is_polling = false;
      m_spatial_ctrl.fd = -1;
    }
}


void timer_send_data(union sigval sig){
    if(m_spatial_ctrl.fd > 0)
        send_event(m_spatial_ctrl.fd,m_spatial_ctrl.ret);
}

/*
 * Class:     com_android_bluetooth_ble_app_headset_Spatialcontroller
 * Method:    createspatialhid
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_android_bluetooth_ble_app_headset_Spatialcontroller_createspatialhid
  (JNIEnv *env, jclass, jstring addrObj) {
  const char *addr = env->GetStringUTFChars(addrObj, 0);
  if(strcmp(m_addr,addr) == 0 && m_spatial_ctrl.fd > 0) {
    LOGD("device is same ignore connect");
    env->ReleaseStringUTFChars(addrObj, addr);
    return true;
  }
  memcpy((void*)m_addr, (void*)addr, sizeof(char)*ADDR_STR_LENGTH);
  env->ReleaseStringUTFChars(addrObj, addr);
  return create_hid_point();
  }

/*
 * Class:     com_android_bluetooth_ble_app_headset_Spatialcontroller
 * Method:    sendspatialdata
 * Signature: (Ljava/lang/String;[B)V
 */
JNIEXPORT void JNICALL Java_com_android_bluetooth_ble_app_headset_Spatialcontroller_sendspatialdata
  (JNIEnv *env, jobject, jstring addrObj, jfloatArray floatObj) {
    jfloat* gyroData = (jfloat*)env->GetFloatArrayElements(floatObj, 0);
    m_spatial_ctrl.ret[0] = (int16_t)(((int)gyroData[1])*32767/180);
    m_spatial_ctrl.ret[1] = (int16_t)(((int)gyroData[2])*32767/180);
    m_spatial_ctrl.ret[2] = (int16_t)(((int)gyroData[0])*32767/180);

    for(int i = 0; i< 5; i++) {
        if(i > 2)
           m_spatial_ctrl.ret[i] = 0;
    }
    if(m_spatial_ctrl.fd < 0) {
      const char *addr = env->GetStringUTFChars(addrObj, 0);
      memcpy((void*)m_addr, (void*)addr, sizeof(char)*ADDR_STR_LENGTH);
      env->ReleaseStringUTFChars(addrObj, addr);
      create_hid_point();
    } else {
        send_event(m_spatial_ctrl.fd,m_spatial_ctrl.ret);
    }
    env->ReleaseFloatArrayElements( floatObj, gyroData, 0);
}

/*
 * Class:     com_android_bluetooth_ble_app_headset_Spatialcontroller
 * Method:    destroyspatialhid
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_android_bluetooth_ble_app_headset_Spatialcontroller_destroyspatialhid
  (JNIEnv *, jclass) {
  destroy_hid_point();
  return true;
}

#ifdef __cplusplus
}
#endif
