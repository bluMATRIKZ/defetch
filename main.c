#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef HAVE_STRDUP
char *my_strdup(const char *s)
{
    size_t n;
    char *p;

    if (!s) {
        return NULL;
    }
    n = strlen(s) + 1;
    p = (char *)malloc(n);
    if (p) {
        memcpy(p, s, n);
    }
    return p;
}
#define strdup(s) my_strdup(s)
#endif

int command_exists(const char *cmd) {
    char *env_path;
    char *path_copy;
    char *dir;
    char fullpath[512];
    int found = 0;

    env_path = getenv("PATH");
    if (!env_path) {
        return 0;
    }

    path_copy = strdup(env_path);
    if (!path_copy) {
        return 0;
    }

    dir = strtok(path_copy, ":");
    while (dir != NULL) {
        if (snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd) < 0) {
            dir = strtok(NULL, ":");
            continue;
        }

        if (access(fullpath, X_OK) == 0) {
            found = 1;
            break;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return found;
}

void print(const char *label, const char *value) {
    if (value && *value) {
        printf("%-15s %s\n", label, value);
    }
}

void get_cpu(void); 

void get_line_val(const char *path, const char *key, char *out, size_t len) {
    FILE *f;
    char line[256];
    char *val_start;
    char *val_end;
    size_t key_len = strlen(key);
    size_t copy_len;

    *out = '\0';

    f = fopen(path, "r");
    if (!f) {
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, key_len) == 0 && line[key_len] == '=') {
            val_start = &line[key_len + 1];

            while (*val_start == ' ' || *val_start == '\t') {
                val_start++;
            }

            if (*val_start == '"') {
                val_start++;
            }

            val_end = val_start;
            while (*val_end != '\0' && *val_end != '\n' && *val_end != '\r') {
                val_end++;
            }
            if (val_end > val_start && *(val_end - 1) == '"') {
                val_end--;
            }

            copy_len = val_end - val_start;
            if (copy_len >= len) {
                copy_len = len - 1;
            }

            strncpy(out, val_start, copy_len);
            out[copy_len] = '\0';
            break;
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
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = '\0';
            print("Host:", buf);
        }
        fclose(f);
    }
}

void get_kernel() {
    FILE *f;
    char buf[128];

    f = fopen("/proc/sys/kernel/osrelease", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = '\0';
            print("Kernel:", buf);
        }
        fclose(f);
    }
}

void get_uptime() {
    FILE *f;
    double uptime_seconds;
    long seconds;
    int years, months, days, hours, minutes;
    char out[256];
    int first_component = 1;

    f = fopen("/proc/uptime", "r");
    if (f) {
        if (fscanf(f, "%lf", &uptime_seconds) == 1) {
            seconds = (long)uptime_seconds;

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
                first_component = 0;
            }
            if (months > 0) {
                if (!first_component) {
                    strcat(out, ", ");
                }
                sprintf(out + strlen(out), "%d month%s", months, months == 1 ? "" : "s");
                first_component = 0;
            }
            if (days > 0) {
                if (!first_component) {
                    strcat(out, ", ");
                }
                sprintf(out + strlen(out), "%d day%s", days, days == 1 ? "" : "s");
                first_component = 0;
            }
            if (hours > 0) {
                if (!first_component) {
                    strcat(out, ", ");
                }
                sprintf(out + strlen(out), "%d hour%s", hours, hours == 1 ? "" : "s");
                first_component = 0;
            }
            if (minutes > 0) {
                if (!first_component) {
                    strcat(out, ", ");
                }
                sprintf(out + strlen(out), "%d min%s", minutes, minutes == 1 ? "" : "s");
                first_component = 0;
            }

            if (first_component) {
                sprintf(out, "0 min");
            }
            print("Uptime:", out);
        }
        fclose(f);
    }
}

void get_shell() {
    char *shell = getenv("SHELL");
    const char *base;
    char cmd[128];
    char version[128];
    char out[256];
    FILE *f;
    char *v_ptr;

    if (!shell) {
        return;
    }

    base = strrchr(shell, '/');
    base = base ? base + 1 : shell;

    if (snprintf(cmd, sizeof(cmd), "%s --version 2>/dev/null", shell) < 0) {
        print("Shell:", base);
        return;
    }

    f = popen(cmd, "r");
    if (f) {
        if (fgets(version, sizeof(version), f)) {
            version[strcspn(version, "\n")] = '\0';

            v_ptr = strstr(version, base);
            if (v_ptr) {
                v_ptr += strlen(base);
            } else {
                v_ptr = version;
            }

            while (*v_ptr == ' ' || *v_ptr == ',' || *v_ptr == '-') {
                v_ptr++;
            }

            if (snprintf(out, sizeof(out), "%s %s", base, v_ptr) >= 0) {
                print("Shell:", out);
            } else {
                print("Shell:", base);
            }
        } else {
            print("Shell:", base);
        }
        pclose(f);
    } else {
        print("Shell:", base);
    }
}

void get_cpu_info() {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) return;

    char line[256];
    char *model_name = NULL;
    int physical_ids[256] = {0};
    int core_ids[256][256] = {{0}};
    int total_threads = 0;
    int physical_count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0 && !model_name) {
            char *start = strchr(line, ':');
            if (start) {
                start += 2;
                model_name = strdup(start);
                model_name[strcspn(model_name, "\n")] = '\0';
            }
        }

        static int current_phys_id = -1;
        static int current_core_id = -1;

        if (strncmp(line, "physical id", 11) == 0) {
            sscanf(line, "physical id\t: %d", &current_phys_id);
        } else if (strncmp(line, "core id", 7) == 0) {
            sscanf(line, "core id\t: %d", &current_core_id);
            if (!core_ids[current_phys_id][current_core_id]) {
                core_ids[current_phys_id][current_core_id] = 1;
                physical_count++;
            }
        } else if (strncmp(line, "processor", 9) == 0) {
            total_threads++;
        }
    }
    fclose(f);

    if (model_name) {
        printf("%s (%dC) (%dT)\n", model_name, physical_count, total_threads);
        free(model_name);
    }
}

void get_gpu() {
    FILE *f;
    char buf[256];

    f = popen("lspci 2>/dev/null | grep -i 'vga' | sed 's/.*: //'", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = '\0';
            print("GPU:", buf);
        } else {
            print("GPU:", "n/a");
        }
        pclose(f);
    } else {
        print("GPU:", "n/a");
    }
}

void get_memory() {
    FILE *f = fopen("/proc/meminfo", "r");
    char label[32], discard_char;
    long value_kb;
    long total_kb = -1, avail_kb = -1;
    char mem_str[64];
    double used_mib, total_mib;
    int percent_used;

    if (!f) {
        return;
    }

    while (fscanf(f, "%31s %ld %c", label, &value_kb, &discard_char) == 3) {
        if (strcmp(label, "MemTotal:") == 0) {
            total_kb = value_kb;
        } else if (strcmp(label, "MemAvailable:") == 0) {
            avail_kb = value_kb;
        }
        if (total_kb != -1 && avail_kb != -1) {
            break;
        }
        while (fgetc(f) != '\n' && !feof(f));
    }

    fclose(f);

    if (total_kb > 0 && avail_kb >= 0) {
        used_mib = (double)(total_kb - avail_kb) / 1024.0;
        total_mib = (double)total_kb / 1024.0;
        percent_used = (int)((used_mib * 100.0) / total_mib);
        if (snprintf(mem_str, sizeof(mem_str), "%.0f MiB / %.0f MiB (%d%%)", used_mib, total_mib, percent_used) >= 0) {
            print("Memory:", mem_str);
        }
    }
}

void get_resolution() {
    FILE *f = popen("xrandr 2>/dev/null | grep '*' | awk '{print $1}'", "r");
    char buf[32];

    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = '\0';
            print("Resolution:", buf);
        }
        pclose(f);
    }
}

void get_storage() {
    FILE *df_pipe = popen("df -B1 / | tail -1", "r");
    char line[256];
    char out[64];
    unsigned long long total_bytes, used_bytes;
    int percent_disk;

    if (df_pipe) {
        if (fgets(line, sizeof(line), df_pipe)) {
            if (sscanf(line, "%*s %llu %llu", &total_bytes, &used_bytes) == 2) {
                double total_gib = total_bytes / (1024.0 * 1024.0 * 1024.0);
                double used_gib = used_bytes / (1024.0 * 1024.0 * 1024.0);
                percent_disk = (int)((used_gib / total_gib) * 100.0);
                if (snprintf(out, sizeof(out), "%.2f GiB / %.2f GiB (%d%%)", used_gib, total_gib, percent_disk) >= 0) {
                    print("Disk:", out);
                }
            }
        }
        pclose(df_pipe);
    }

    {
        FILE *mem_file = fopen("/proc/meminfo", "r");
        char buf_mem[128];
        long swap_total_kb = 0, swap_free_kb = 0;
        int percent_swap;
        long used_swap_kb;

        if (mem_file) {
            while (fgets(buf_mem, sizeof(buf_mem), mem_file)) {
                if (sscanf(buf_mem, "SwapTotal: %ld kB", &swap_total_kb) == 1) {
                } else if (sscanf(buf_mem, "SwapFree: %ld kB", &swap_free_kb) == 1) {
                    break;
                }
            }
            fclose(mem_file);
        }

        if (swap_total_kb > 0) {
            used_swap_kb = swap_total_kb - swap_free_kb;
            percent_swap = (int)(((double)used_swap_kb * 100.0) / swap_total_kb);
            if (snprintf(out, sizeof(out), "%ld MiB / %ld MiB (%d%%)",
                         used_swap_kb / 1024, swap_total_kb / 1024, percent_swap) >= 0) {
                print("Swap:", out);
            }
        }
    }
}

void get_de() {
    char *de = getenv("XDG_CURRENT_DESKTOP");
    print("DE:", de);
}

void get_wm() {
    FILE *f;
    char buf[64];

    f = popen("wmctrl -m 2>/dev/null | grep Name | cut -d: -f2", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            char *trimmed_buf = buf;
            while (*trimmed_buf == ' ') {
                trimmed_buf++;
            }
            trimmed_buf[strcspn(trimmed_buf, "\n")] = '\0';
            print("WM:", trimmed_buf);
        }
        pclose(f);
    }
}

void get_terminal() {
    FILE *f;
    char buf[64];

    f = popen("ps -o comm= -p $(ps -o ppid= -p $(ps -o ppid= -p $$))", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = '\0';
            print("Terminal:", buf);
        }
        pclose(f);
    }
}

void get_packages() {
    struct pkg {
        const char *cmd;
        const char *count_cmd;
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

    char pkg_str[256];
    char buf[64];
    FILE *f_pipe;
    int found_any_pm = 0;
    int i;
    int num_pkgs;

    pkg_str[0] = '\0';

    for (i = 0; i < (int)(sizeof(pkgs) / sizeof(pkgs[0])); i++) {
        if (command_exists(pkgs[i].cmd)) {
            f_pipe = popen(pkgs[i].count_cmd, "r");
            if (f_pipe) {
                if (fgets(buf, sizeof(buf), f_pipe)) {
                    num_pkgs = atoi(buf) - pkgs[i].adjust;
                    if (num_pkgs > 0) {
                        if (found_any_pm) {
                            strcat(pkg_str, ", ");
                        }
                        char tmp[64];
                        if (snprintf(tmp, sizeof(tmp), "%d (%s)", num_pkgs, pkgs[i].label) >= 0) {
                            strcat(pkg_str, tmp);
                        }
                        found_any_pm++;
                    }
                }
                pclose(f_pipe);
            }
        }
    }

    if (!found_any_pm) {
        strcpy(pkg_str, "none");
    }
    print("Packages:", pkg_str);
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
    get_storage();

    return 0;
}
