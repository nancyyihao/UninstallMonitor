#include <stdio.h>  
#include <jni.h>  
#include <stdlib.h>
#include <unistd.h>  
#include <sys/inotify.h>  
#include "Log.h"
#include "find_pid_by_name.h"
#include "utils.h"
#include <sys/prctl.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
const char monitorProcess[] = "ntes_uninstall";

void initPkgDir(char *pkgDir, const char *pkgName) {
    strcpy(pkgDir, "/data/data/");
    strcat(pkgDir, pkgName);
}

void clearOldMonitorProcess(char *monitorProcessName) {
    int pids_lyman[128];
    int i, iskill, rv;
    rv = find_pid_by_name_status(monitorProcessName, pids_lyman);
    if (!rv) {
        if (pids_lyman[0] == 0)
            LOGI("have no old process!");

        for (i = 0; pids_lyman[i] != 0; i++) {
            LOGI("oldMonitorProcess pid=%d ", pids_lyman[i]);
            iskill = kill(pids_lyman[i], SIGKILL);

            if (iskill) {
                LOGI("kill oldMonitorProcess pid=%d failed!", pids_lyman[i]);
            } else {
                LOGI("kill oldMonitorProcess pid=%d sucess!", pids_lyman[i]);
            }

        }

    } else {
        LOGE("clear oldMonitorProcess failed!");
    }
}

void execStartUrl(JNIEnv *env, int sdkVersion, char *uninstallInquireUrl) {
//execlp()会从PATH 环境变量所指的目录中查找符合参数file的文件名，找到后便执行该文件，然后将第二个以后的参数当做该文件的argv[0]、argv[1]……，最后一个参数必须用空指针(NULL)作结束
    LOGD("begin exec");
    if (sdkVersion >= 17) {
        // Android4.2系统之后支持多用户操作，所以得指定用户a
        execlp("am", "am", "start", "--user", "0", "-a", "android.intent.action.VIEW", "-d",
               uninstallInquireUrl, (char *) NULL);
    } else {
        // Android4.2以前的版本无需指定用户
        execlp("am", "am", "start", "-a", "android.intent.action.VIEW", "-d", uninstallInquireUrl,
               (char *) NULL);
    }
    LOGD("end exec");
}

JNIEXPORT jint JNICALL
Java_com_netease_nr_base_activity_UnInstallMonitor_monitorByPoll(JNIEnv *env, jclass type,
                                                                 jstring pkgName_,
                                                                 jstring url_, jint jsdkVersion) {
    const char *pkg = jstringTostring(env, pkgName_);
    const char *url = jstringTostring(env, url_);
    char pkgDir[128];
    initPkgDir(pkgDir, pkg);
    clearOldMonitorProcess(monitorProcess);
    //初始化log
    LOGD("init!pkgDir:%s;monitorProcess:%s", pkgDir, monitorProcess);
    //fork子进程，以执行轮询任务
    pid_t pid = fork();
    if (pid < 0) {
        //出错log
        LOGD("fork error !!!");
    }
    else if (pid == 0) {
        //设置进程名
        prctl(PR_SET_NAME, monitorProcess);
        //子进程轮询"/data/data/com.example.untitled"目录是否存在，若不存在则说明已被卸载
        while (1) {
            FILE *p_file = fopen(pkgDir, "r");
            if (p_file == NULL) {
                execStartUrl(env, jsdkVersion, url);
            } else {
                sleep(1);

            }
            fclose(p_file);
        }
    }
    else {
        //父进程直接退出，使子进程被init进程领养，以避免子进程僵死
    }
    return (*env)->NewStringUTF(env, "Hello from JNI !");
}


JNIEXPORT jint JNICALL
Java_com_netease_nr_base_activity_UnInstallMonitor_monitorByNotify(JNIEnv *env, jclass type,
                                                                   jstring pkgName_,
                                                                   jstring url_, jint jsdkVersion) {
    const char *pkg = jstringTostring(env, pkgName_);
    const char *url = jstringTostring(env, url_);
    char pkgDir[128];
    initPkgDir(pkgDir, pkg);
    clearOldMonitorProcess(monitorProcess);
    //初始化log
    LOGD("init!pkgDir:%s;monitorProcess:%s", pkgDir, monitorProcess);

    //fork子进程，以执行轮询任务
    pid_t pid = fork();
    if (pid < 0) {
        //出错log
        LOGE("fork failed !!!");
    } else if (pid == 0) {
        //设置进程名
        prctl(PR_SET_NAME, monitorProcess);
        //子进程注册目录监听器
        int fileDescriptor = inotify_init();
        if (fileDescriptor < 0) {
            LOGD("inotify_init failed !!!");
            exit(1);
        }
//        LOGD("inotify_init fileDescriptor:%d", fileDescriptor);
        int watchDescriptor;
        watchDescriptor = inotify_add_watch(fileDescriptor, pkgDir,
                                            IN_DELETE);
//        LOGD("inotify_init watchDescriptor:%d", watchDescriptor);
        if (watchDescriptor < 0) {
            LOGD("inotify_add_watch failed !!!");
            exit(1);
        }
        char buff[BUF_LEN];
        //开始监听
        LOGD("start observer");
        while (1) {
            int readBytes = read(fileDescriptor, buff, BUF_LEN);
//            LOGD("readBytes %d", readBytes);
            //read会阻塞进程，走到这里说明收到目录被删除的事件
            FILE *file = fopen(pkgDir, "r");
            if (file) {
//                LOGD("file is not null! %d", file);
                fclose(file);
//                sleep(1);
                continue;
            } else {
                //目录不存在
                LOGD("uninstalled");
                execStartUrl(env, jsdkVersion, url);
                inotify_rm_watch(fileDescriptor, IN_DELETE);
                break;
            }
        }
    } else {
        //父进程直接退出，使子进程被init进程领养，以避免子进程僵死
        LOGD("sub process pid =%d", pid);
    }

    return (*env)->NewStringUTF(env, "Hello from JNI !");
}
