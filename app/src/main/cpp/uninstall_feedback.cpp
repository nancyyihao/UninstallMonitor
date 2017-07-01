#include <stdio.h>
#include <jni.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/prctl.h>          /*for prctl function*/
#include <dirent.h>             /*for DIR struct*/
#include <android/log.h>


#define LOG_TAG "uninstall_tag"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C"
{

char monitorProcessName[] = "ntes_uninstall";

void loadUrl(int sdkVersion, const char *url) {
    LOGD("start open url....");
    if (sdkVersion < 17) {
        execlp("am", "am", "start", "-a", "android.intent.action.VIEW", "-d", url, NULL);
    } else {
        execlp("am", "am", "start", "--user", "0", "-a",
               "android.intent.action.VIEW", "-d", url, (char *) NULL);
    }
    LOGD("stop open url....");
}

int find_pid_by_name(const char *ProcName, int *foundpid) {
    DIR *dir;
    struct dirent *d;
    int pid, i;
    char *s;
    int pnlen;

    i = 0;
    foundpid[0] = 0;
    pnlen = strlen(ProcName);

    /* Open the /proc directory. */
    dir = opendir("/proc");
    if (!dir) {
        LOGE("cannot open /proc");
        return -1;
    }

    /* Walk through the directory. */
    while ((d = readdir(dir)) != NULL) {

        char exe[PATH_MAX + 1];
        char path[PATH_MAX + 1];
        int len;
        int namelen;

        /* See if this is a process */
        if ((pid = atoi(d->d_name)) == 0) continue;

        snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
        if ((len = readlink(exe, path, PATH_MAX)) < 0)
            continue;
        path[len] = '\0';

        /* Find ProcName */
        s = strrchr(path, '/');
        if (s == NULL) continue;
        s++;

        /* we don't need small name len */
        namelen = strlen(s);
        if (namelen < pnlen) continue;

        if (!strncmp(ProcName, s, pnlen)) {
            /* to avoid subname like search proc tao but proc taolinke matched */
            if (s[pnlen] == ' ' || s[pnlen] == '\0') {
                foundpid[i] = pid;
                i++;
            }
        }
    }

    foundpid[i] = 0;
    closedir(dir);

    return 0;

}

void clearOldProcessByName(const char *monitorProcessName) {

    int i, killFailed, findResult, oldPids[128];

    findResult = find_pid_by_name(monitorProcessName, oldPids);
    if (!findResult) {
        if (oldPids[0] == 0)
            LOGD("have no old process!");

        for (i = 0; oldPids[i] != 0; i++) {
            LOGD("old process pid=%d ", oldPids[i]);
            // return zero if kill process successfully
            killFailed = kill(oldPids[i], SIGKILL);

            if (killFailed) {
                LOGD("kill old process pid=%d failed!", oldPids[i]);
            } else {
                LOGD("kill old process pid=%d success!", oldPids[i]);
            }
        }

    } else {
        LOGE("clear old process failed!");
    }
}

JNIEXPORT void JNICALL
Java_com_netease_uninstallmonitor_MainActivity_monitorUsingPoll(JNIEnv *env, jobject instance,
                                                                jstring pkgDir_, jint sdkVersion_,
                                                                jstring openUrl_) {
    const char *pkgDir = env->GetStringUTFChars(pkgDir_, 0);
    const char *openUrl = env->GetStringUTFChars(openUrl_, 0);

    // need to clear old process
    clearOldProcessByName(monitorProcessName);

    int state = fork();
    if (state > 0) {
        LOGD("parent process = %d", state);
    } else if (state == 0) {
        LOGD("child process = %d", state);
        int stop = 0;
        while (!stop) {
            sleep(1);
            FILE *file = fopen(pkgDir, "r");
            if (file == NULL) {
                LOGD("app uninstalled already!");
                loadUrl(sdkVersion_, openUrl);
                stop = 1;
            }
        }
    } else {
        LOGD("Error");
    }

    env->ReleaseStringUTFChars(pkgDir_, pkgDir);
    env->ReleaseStringUTFChars(openUrl_, openUrl);
}

JNIEXPORT void JNICALL
Java_com_netease_uninstallmonitor_MainActivity_monitorUsingNotify(JNIEnv *env, jobject instance,
                                                                  jstring pkgDir_, jint sdkVersion,
                                                                  jstring openUrl_) {
    const char *pkgDir = env->GetStringUTFChars(pkgDir_, 0);
    const char *openUrl = env->GetStringUTFChars(openUrl_, 0);

    // need to clear old process
    clearOldProcessByName(monitorProcessName);

    int pid = fork();
    if (pid > 0) {
        LOGD("parent process = %d", pid);
    } else if (pid == 0) {
        LOGD("child process = %d", pid);
        prctl(PR_SET_NAME, monitorProcessName);
        // register notify listener
        int fd = inotify_init();
        if (fd < 0) {
            LOGD("inofity_init failed!!!");
            exit(1);
        }

        int wd = inotify_add_watch(fd, pkgDir, IN_DELETE);
        if (wd < 0) {
            LOGD("inotify_add_watch failed !!!");
            exit(1);
        }

        // handle an event once a time
        void *p_buf = malloc(sizeof(inotify_event));
        if (p_buf == NULL) {
            LOGD("malloc failed!!!");
            exit(1);
        }

        LOGD("start listen");

        // block session until file delete
        read(fd, p_buf, sizeof(struct inotify_event));
        free(p_buf);
        inotify_rm_watch(fd, IN_DELETE);
        loadUrl(sdkVersion, openUrl);

        LOGD("end listen");

    } else {
        LOGD("Error");
    }

    env->ReleaseStringUTFChars(pkgDir_, pkgDir);
    env->ReleaseStringUTFChars(openUrl_, openUrl);
}

}

