#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include "base/daemon.h"
#include "error_code.h"
namespace tink {
    static int CheckPid(std::string_view pid_file) {
        int pid = 0;
        FILE *f = fopen(pid_file.data(),"r");
        if (f == nullptr)
            return 0;
        int n = fscanf(f,"%d", &pid);
        fclose(f);

        if (n !=1 || pid == 0 || pid == getpid()) {
            return 0;
        }

        if (kill(pid, 0) && errno == ESRCH)
            return 0;

        return pid;

    }

    static int WritePid(std::string_view pid_file) {
        int pid = 0;
        int fd = open(pid_file.data(), O_RDWR|O_CREAT, 0644);
        if (fd == -1) {
            fprintf(stderr, "Can't create pidfile [%s].\n", pid_file.data());
            return 0;
        }
        FILE *f = fdopen(fd, "w+");
        if (f == nullptr) {
            fprintf(stderr, "Can't open pidfile [%s].\n", pid_file.data());
            return 0;
        }

        if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
            int n = fscanf(f, "%d", &pid);
            fclose(f);
            if (n != 1) {
                fprintf(stderr, "Can't lock and read pidfile.\n");
            } else {
                fprintf(stderr, "Can't lock pidfile, lock is held by pid %d.\n", pid);
            }
            return 0;
        }

        pid = getpid();
        if (!fprintf(f,"%d\n", pid)) {
            fprintf(stderr, "Can't write pid.\n");
            close(fd);
            return 0;
        }
        fflush(f);

        return pid;
    }

    static int RedirectFds() {
        int nfd = open("/dev/null", O_RDWR);
        if (nfd == -1) {
            perror("Unable to open /dev/null: ");
            return -1;
        }
        if (dup2(nfd, 0) < 0) {
            perror("Unable to dup2 stdin(0): ");
            return -1;
        }
        if (dup2(nfd, 1) < 0) {
            perror("Unable to dup2 stdout(1): ");
            return -1;
        }
        if (dup2(nfd, 2) < 0) {
            perror("Unable to dup2 stderr(2): ");
            return -1;
        }
        close(nfd);
        return 0;
    }

    int Daemon::Init(std::string_view pid_file) {
        if (int pid = CheckPid(pid_file)) {
            fprintf(stderr, "Server is already running, pid = %d.\n", pid);
            return E_FAILED;
        }

        if (daemon(1,1)) {
            fprintf(stderr, "Can't daemonize.\n");
            return E_FAILED;
        }
        if (WritePid(pid_file) == 0) {
            return E_FAILED;
        }
        if (RedirectFds()) {
            return E_FAILED;
        }

        return E_OK;
    }

    int Daemon::Exit(std::string_view pid_file) {
        return unlink(pid_file.data());
    }

}

