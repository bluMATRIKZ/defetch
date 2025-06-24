#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef HAVE_STRDUP
char *my_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}
#define strdup(s) my_strdup(s)
#endif

int command_exists(const char *cmd) {
    char *env_path;
    char *path;
    char *dir;
    char fullpath[512];
    int found = 0;

    env_path = getenv("PATH");
    if (!env_path)
        return 0;
    path = strdup(env_path);
    if (!path)
        return 0;
    dir = strtok(path, ":");
    while (dir != NULL) {
        sprintf(fullpath, "%s/%s", dir, cmd);
        if (access(fullpath, X_OK) == 0) {
            found = 1;
            break;
        }
        dir = strtok(NULL, ":");
    }
    free(path);
    return found;
}

void print(const char *label, const char *value) {
    if (value && *value)
        printf("%-15s %s\n", label, value);
}

void get_line_val(const char *path, const char *key, char *out, size_t len) {
    FILE *f;
    char line[256];
    char *val;
    f = fopen(path, "r");
    if (!f)
        return;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            val = strchr(line, '=');
            if (val) {
                val++; 
                val[strcspn(val, "\n")] = 0;
                if (*val == '"')
                    val++;
                if (strlen(val) > 0 && val[strlen(val)-1] == '"')
                    val[strlen(val)-1] = 0;
                strncpy(out, val, len - 1);
                out[len - 1] = 0;
                break;
            }
        }
    }
    fclose(f);
}

void get_os() {
    char buf[128];
    get_line_val("/etc/os-release", "PRETTY_NAME", buf, sizeof(buf));
    print("OS:", buf);
}

void get_host() {
    FILE *f;
    char buf[128];
    f = fopen("/sys/class/dmi/id/product_name", "r");
    if (f && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = 0;
        print("Host:", buf);
        fclose(f);
    }
}

void get_kernel() {
    FILE *f;
    char buf[128];
    f = fopen("/proc/sys/kernel/osrelease", "r");
    if (f && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = 0;
        print("Kernel:", buf);
        fclose(f);
    }
}

void get_uptime() {
    FILE *f;
    double uptime_seconds;
    long seconds;
    int years, months, days, hours, minutes;
    char out[256];
    int first = 1;
    
    f = fopen("/proc/uptime", "r");
    if (f) {
        if (fscanf(f, "%lf", &uptime_seconds) == 1) {
            seconds = (long) uptime_seconds;
            years   = seconds / 31536000;
            seconds %= 31536000;
            months  = seconds / 2592000;
            seconds %= 2592000;
            days    = seconds / 86400;
            seconds %= 86400;
            hours   = seconds / 3600;
            seconds %= 3600;
            minutes = seconds / 60;
            
            out[0] = '\0';
            if (years > 0) {
                sprintf(out + strlen(out), "%d year%s", years, years == 1 ? "" : "s");
                first = 0;
            }
            if (months > 0) {
                if (!first)
                    strcat(out, ", ");
                sprintf(out + strlen(out), "%d month%s", months, months == 1 ? "" : "s");
                first = 0;
            }
            if (days > 0) {
                if (!first)
                    strcat(out, ", ");
                sprintf(out + strlen(out), "%d day%s", days, days == 1 ? "" : "s");
                first = 0;
            }
            if (hours > 0) {
                if (!first)
                    strcat(out, ", ");
                sprintf(out + strlen(out), "%d hour%s", hours, hours == 1 ? "" : "s");
                first = 0;
            }
            if (minutes > 0) {
                if (!first)
                    strcat(out, ", ");
                sprintf(out + strlen(out), "%d min%s", minutes, minutes == 1 ? "" : "s");
                first = 0;
            }
            if (first) {
                sprintf(out, "0 min");
            }
            print("Uptime:", out);
        }
        fclose(f);
    }
}

void get_shell() {
    char *shell;
    const char *base;
    char cmd[256];
    char version[128];
    char out[256];
    FILE *f;
    
    shell = getenv("SHELL");
    if (!shell)
        return;
    base = strrchr(shell, '/');
    if (base)
        base++;
    else
        base = shell;
    sprintf(cmd, "%s --version 2>/dev/null | head -n1 | awk '{print $2}'", shell);
    f = popen(cmd, "r");
    if (f && fgets(version, sizeof(version), f)) {
        version[strcspn(version, "\n")] = 0;
        sprintf(out, "%s %s", base, version);
        print("Shell:", out);
        pclose(f);
    } else {
        print("Shell:", base);
    }
}

void get_cpu() {
    FILE *f;
    char line[256];
    char *v;
    
    f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0) {
            v = strchr(line, ':');
            if (v) {
                v += 2;
                v[strcspn(v, "\n")] = 0;
                print("CPU:", v);
            }
            break;
        }
    }
    fclose(f);
}

void get_gpu() {
    FILE *f;
    char buf[256];
    
    f = popen("lspci 2>/dev/null | grep -i 'vga' | sed 's/.*: //'", "r");
    if (f && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = 0;
        print("GPU:", buf);
        pclose(f);
    } else {
        print("GPU:", "n/a");
    }
}

void get_memory() {
    FILE *f;
    char label[32];
    char discard;
    long val;
    long total = -1, avail = -1;
    char memStr[64];
    double used, total_mib;
    
    f = fopen("/proc/meminfo", "r");
    if (!f)
        return;
    while (fscanf(f, "%31s %ld %c", label, &val, &discard) == 3) {
        if (strcmp(label, "MemTotal:") == 0)
            total = val;
        else if (strcmp(label, "MemAvailable:") == 0)
            avail = val;
        if (total != -1 && avail != -1)
            break;
        fgets(label, sizeof(label), f);
    }
    fclose(f);
    if (total != -1 && avail != -1) {
        used = (total - avail) / 1024.0;
        total_mib = total / 1024.0;
        sprintf(memStr, "%.0f MiB / %.0f MiB", used, total_mib);
        print("Memory:", memStr);
    }
}

void get_resolution() {
    FILE *f;
    char buf[32];
    
    f = popen("xrandr 2>/dev/null | grep '*' | awk '{print $1}'", "r");
    if (f && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = 0;
        print("Resolution:", buf);
        pclose(f);
    }
}

void get_de() {
    char *de;
    de = getenv("XDG_CURRENT_DESKTOP");
    print("DE:", de);
}

void get_wm() {
    FILE *f;
    char buf[64];
    
    f = popen("wmctrl -m 2>/dev/null | grep Name | cut -d: -f2", "r");
    if (f && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = 0;
        print("WM:", buf);
        pclose(f);
    }
}

void get_terminal() {
    FILE *f;
    char buf[64];
    
    f = popen("ps -o comm= -p $(ps -o ppid= -p $(ps -o ppid= -p $$))", "r");
    if (f && fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\n")] = 0;
        print("Terminal:", buf);
        pclose(f);
    }
}

void get_packages() {
    struct pkg {
        const char *cmd;
        const char *count;
        const char *label;
        int adjust;
    } pkgs[] = {
        {"dpkg",       "dpkg -l | wc -l",                      "dpkg",    0},
        {"rpm",        "rpm -qa | wc -l",                      "rpm",     0},
        {"pacman",     "pacman -Q | wc -l",                    "pacman",  0},
        {"apk",        "apk info | wc -l",                     "apk",     0},
        {"xbps-query", "xbps-query -l | wc -l",                "xbps",    0},
        {"flatpak",    "flatpak list | wc -l",                 "flatpak", 0},
        {"snap",       "snap list | wc -l",                    "snap",    1},
        {"zypper",     "zypper se -i | wc -l",                 "zypper",  0},
        {"dnf",        "dnf list installed | wc -l",           "dnf",     0},
        {"yum",        "yum list installed | wc -l",           "yum",     0},
        {"qlist",      "qlist -I | wc -l",                     "emerge",  0},
        {"guix",       "guix package -I | wc -l",              "guix",    0},
        {"nix-store",  "nix-store --gc --print-roots | wc -l", "nix",     0}
    };
    char pkgStr[256];
    char buf[64];
    char cmdcheck[128];
    int found = 0;
    int i;
    FILE *f;
    
    pkgStr[0] = '\0';
    for (i = 0; i < (int)(sizeof(pkgs) / sizeof(pkgs[0])); i++) {
        sprintf(cmdcheck, "command -v %s > /dev/null 2>&1", pkgs[i].cmd);
        if (!system(cmdcheck)) {
            f = popen(pkgs[i].count, "r");
            if (f && fgets(buf, sizeof(buf), f)) {
                int n = atoi(buf) - pkgs[i].adjust;
                if (n > 0) {
                    if (found)
                        strcat(pkgStr, ", ");
                    {
                        char tmp[64];
                        sprintf(tmp, "%d (%s)", n, pkgs[i].label);
                        strcat(pkgStr, tmp);
                    }
                    found++;
                }
                pclose(f);
            }
        }
    }
    if (!found)
        strcpy(pkgStr, "none");
    print("Packages:", pkgStr);
}

int main() {
    get_os();
    get_host();
    get_kernel();
    get_uptime();
    get_shell();
    get_packages();
    get_resolution();
    get_de();
    get_wm();
    get_terminal();
    get_cpu();
    get_gpu();
    get_memory();
    return 0;
}
