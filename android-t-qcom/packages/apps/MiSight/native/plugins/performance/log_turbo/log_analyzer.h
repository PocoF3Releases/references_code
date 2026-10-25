#ifndef _LOG_ANALYZER_
#define _LOG_ANALYZER_
#include <string>
#include <android-base/macros.h>
#include "event_source.h"
#include "plugin.h"

//event_id sync with PerfDebugMonitorImpl.java
//App msg running long
static const int PERF_EVENT_ID_APP_LONG_MSG_RUNNING = 902001001;
//App main thread busy cause activity lifecycle late
static const int PERF_EVENT_ID_ACTIVITY_LATE = 902001002;
//App main thread busy cause do doFrame late
static const int PERF_EVENT_ID_BUSY_MAIN  = 902001003;
//App doFrame long
static const int PERF_EVENT_ID_DO_FRAME = 902001004;
//Long lock in msg
static const int PERF_EVENT_ID_APP_LONG_MSG_LOCK = 902001005;
//App traversals long
static const int PERF_EVENT_ID_TRAVERSALS = 902001006;
//analyze slow method in SurfaceFlinger
static const int PERF_EVENT_ID_SF_SLOW_METHOD = 902001050;
//Thermal cause msg running long
static const int PERF_EVENT_ID_THERMAL_LONG_MSG = 902001100;
//System cause msg runnable long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE = 902001101;
//System cause msg blkio long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO = 902001102;
//System cause msg swapin long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN = 902001103;
//System cause msg reclaim long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM = 902001104;
//Msg call binder, and binder running long
static const int PERF_EVENT_ID_LONG_MSG_BINDER_RUNNING = 902001200;
//Long lock in msg binder interface
static const int PERF_EVENT_ID_LONG_MSG_BINDER_LOCK = 902001201;
//System cause msg binder interface runnable long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE_BINDER = 902001202;
//System cause msg binder interface blkio long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO_BINDER = 902001203;
//System cause msg binder interface swapin long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN_BINDER = 902001204;
//System cause msg binder interface reclaim long
static const int PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM_BINDER = 902001205;
//InputDispatcher dispatch input late
static const int PERF_EVENT_ID_INPUT_SLOW_SERVER = 902002001;
//App busy in handle input
static const int PERF_EVENT_ID_INPUT_SLOW_IN_APP = 902002002;
//App main thread busy cause input late
static const int PERF_EVENT_ID_INPUT_SLOW_BEFORE_APP = 902002003;
//App service onCreate long
static const int PERF_EVENT_ID_SERVICE_SLOW_CREATE = 902001300;
//App main thread busy cause service onCeate long
static const int PERF_EVENT_ID_APP_SERVICE_CREATE_LATE = 902001301;
//App broadcast onReceiver  long
static const int PERF_EVENT_ID_BROADCAST_SLOW_CREATE = 902001302;
//App provider onCreate long
static const int PERF_EVENT_ID_PROVIDER_SLOW_CREATE = 902001303;
//App common method long
static const int PERF_EVENT_ID_COMMON_METHOD = 902049099;
 //Here we use fist ID for next annlyse
static const int PERF_EVENT_FIRST_ID_LONG_MSG = 1001;
static const int PERF_EVENT_FIRST_ID_LONG_BINDER = 1013;
static const int PERF_EVENT_FIRST_ID_LONG_LOCK = 1014;

//event_key sync with Perfdebugmonitorimpl.java
static const std::string  PERF_EVENT_KEY_PLANTIME_CURR = "MsgPlanTime";
static const std::string PERF_EVENT_KEY_PLANTIME_ER = "PlanTimeER";
static const std::string PERF_EVENT_KEY_PLANTIME_UP = "PlanTimeUP";
static const std::string PERF_EVENT_KEY_WALLTIME = "MsgWallTime";
static const std::string PERF_EVENT_KEY_VSYNCFRAME = "VsyncFrame";
static const std::string PERF_EVENT_KEY_LATENCY = "Latency";
static const std::string PERF_EVENT_KEY_PROCSTATE = "ProcState";
static const std::string PERF_EVENT_KEY_PACKAGE = "PackageName";
static const std::string PERF_EVENT_KEY_SEQ = "MsgSeq";
static const std::string PERF_EVENT_KEY_RUNNING_TIME = "RunningTime";
static const std::string PERF_EVENT_KEY_RUNNABLE_TIME = "RunnableTime";
static const std::string PERF_EVENT_KEY_CPU_UTIME = "CpuUTime";
static const std::string PERF_EVENT_KEY_CPU_STIME = "CpuSTime";
static const std::string PERF_EVENT_KEY_IO_TIME =  "IOTime";
static const std::string PERF_EVENT_KEY_SWAPIN_TIME =  "SwapinTime";
static const std::string PERF_EVENT_KEY_RECLAIM_TIME = "ReclaimTime";
static const std::string PERF_EVENT_KEY_TARGET_NAME = "TargetName";
static const std::string PERF_EVENT_KEY_MSG_WHAT = "MsgWhat";
static const std::string PERF_EVENT_KEY_HISTORY_MSG = "HistoryMsg";
static const std::string PERF_EVENT_KEY_MONITOR_TIME = "MonitorTime";
static const std::string PERF_EVENT_KEY_MESSURE_TIME = "MessureTime";
static const std::string PERF_EVENT_KEY_LAYOUT_TIME = "LayoutTime";
static const std::string PERF_EVENT_KEY_DRAW_TIME = "DrawTime";
static const std::string PERF_EVENT_KEY_MDOE = "Mode";
static const std::string PERF_EVENT_KEY_DISPATCH_ON_GLOBAL_LAYOUT = "DispatchTime";
static const std::string PERF_EVENT_KEY_ON_PRE_DRAW_TIME = "OnPreDrawTime";
static const std::string PERF_EVENT_KEY_ON_DRAW_TIME = "OnDrawTime";
static const std::string PERF_EVENT_KEY_VIEW_DRAW_TIME = "ViewDrawTime";
static const std::string PERF_EVENT_KEY_CALLBACKS = "Callback";
static const std::string PERF_EVENT_KEY_TYPE = "Type";
static const std::string PERF_EVENT_KEY_ORDERED = "IsOrder";
static const std::string PERF_EVENT_KEY_INTENT = "BroadcastIntent";
static const std::string PERF_EVENT_KEY_BROADCAST = "Broadcast";
static const std::string PERF_EVENT_KEY_AUTHORITY = "ProviderAuthority";
static const std::string PERF_EVENT_KEY_CLASSNAME = "ProviderClassName";
static const std::string PERF_EVENT_KEY_FLAGS = "Flags";
static const std::string PERF_EVENT_KEY_MULTIPROCESS = "IsMultiprocess";
static const std::string PERF_EVENT_KEY_SERVICETYPE = "ServiceStartType";
static const std::string PERF_EVENT_KEY_PROCESSNAME = "ProcessName";
static const std::string PERF_EVENT_KEY_SERVICENAME = "ServiceName";
static const std::string PERF_EVENT_KEY_FGSERVICETYPE = "FGServiceType";
static const std::string PERF_EVENT_KEY_HISTORY_MSG_COUNT = "HistoryMsgCount";
static const std::string PERF_EVENT_KEY_ACTION = "Action";
static const std::string PERF_EVENT_KEY_CODE = "Code";
static const std::string PERF_EVENT_KEY_EVENT_INFO = "EventInfo";
static const std::string PERF_EVENT_KEY_THREAD_NAME = "ThreadName";
static const std::string PERF_EVENT_KEY_TID = "TID";
static const std::string PERF_EVENT_KEY_UID = "UID";
static const std::string PERF_EVENT_KEY_PID = "PID";
static const std::string PERF_EVENT_KEY_DISPLAY_NAME = "SFDisplayName";
static const std::string PERF_EVENT_KEY_WHERE = "SFDisplayWhere";
static const std::string PERF_EVENT_KEY_DURATION = "Duration";
static const std::string PERF_EVENT_KEY_BINDER_INTERFACE = "BinderInterface";
static const std::string PERF_EVENT_KEY_BINDER_CODE = "BinderCode";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_PID = "BinderTargetPid";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_TID = "BinderTargetTid";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_BLKIO = "BinderTargetBlkio";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_SWAPIN = "BinderTargetSwapin";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_FREEPAGES = "BinderTargetFreepages";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_RUNNING = "BinderTargetRunning";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_RUNNABLE = "BinderTargetRunnable";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_CPU_UTIME = "BinderTargetUTime";
static const std::string PERF_EVENT_KEY_BINDER_TARGET_CPU_STIME = "BinderTargetSTime";
static const std::string PERF_EVENT_KEY_DISPATCH_TIME = "DispatchTime";
static const std::string PERF_EVENT_KEY_FILE_NAME = "FileName";
static const std::string PERF_EVENT_KEY_LINE_NUMBER = "LineNumber";
static const std::string PERF_EVENT_KEY_METHOD_NAME = "FunctionName";
static const std::string PERF_EVENT_KEY_OWNER_FILE_NAME = "HoldLockFileName";
static const std::string PERF_EVENT_KEY_OWNER_LINE_NUMBER = "HoldLockLine";
static const std::string PERF_EVENT_KEY_OWNER_METHOD_NAME = "HoldLockFunction";
static const std::string PERF_EVENT_KEY_EVENT_TIME = "EventTime";
static const std::string PERF_EVENT_KEY_LIMIT_BY_THERMAL = "IsLimitByThermal";
#endif