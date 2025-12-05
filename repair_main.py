
import os

def repair_main():
    with open('main.c', 'r') as f:
        lines = f.readlines()

    new_lines = []
    skip = False
    
    # We want to keep everything up to the end of get_dosbox_version
    # Then insert clean uptime and print_user_host
    # Then skip everything until we hit PSG Sound Functions
    
    clean_functions = """
// Display system uptime (time since midnight/boot)
void uptime(void) {
    unsigned long ticks = get_ticks();
    unsigned long seconds = ticks / 18;  // approx 18.2 ticks/sec
    unsigned int h, m, s;
    char buf[32];
    
    h = (unsigned int)(seconds / 3600);
    seconds -= (unsigned long)h * 3600;
    m = (unsigned int)(seconds / 60);
    s = (unsigned int)(seconds - m * 60);
    
    sprintf(buf, "%uh %um %us", h, m, s);
    _outtext(buf);
}

// Print User@Host header
void print_user_host(void) {
    char *user = getenv("USER");
    char *host = getenv("HOSTNAME");
    int len, i;
    
    // Fallbacks for DOS environment
    if (user == NULL) user = getenv("USERNAME");
    if (user == NULL) user = "User";
    
    if (host == NULL) host = getenv("COMPUTERNAME");
    if (host == NULL) host = "DOS";
    
    
    _settextcolor(LIGHTRED);
    _outtext(user);
    _settextcolor(WHITE);
    _outtext("@");
    _settextcolor(LIGHTRED);
    _outtext(host);
    _outtext("\\n");
    _settextcolor(WHITE);
    
    // Separator line
    len = strlen(user) + strlen(host) + 1;
    if (len > 78) len = 78; // Safety cap
    
    for(i=0; i<len; i++) _outtext("-");
    _outtext("\\n");
}
"""

    found_start = False
    found_end = False
    
    for i, line in enumerate(lines):
        if not skip:
            new_lines.append(line)
            # Check for end of get_dosbox_version
            if "char* get_dosbox_version(void)" in line:
                # We are in the function, wait for closing brace
                pass
            if "return getenv(\"DOSBOX\");" in line:
                # Next line should be '}'
                pass
            if line.strip() == "}" and "return getenv" in lines[i-1]:
                # This is the end of get_dosbox_version
                found_start = True
                skip = True
                new_lines.append(clean_functions)
        else:
            # We are skipping the corrupted mess
            # Look for start of PSG Sound Functions
            if "// PSG Sound Functions" in line:
                skip = False
                found_end = True
                new_lines.append(line)
    
    if found_start and found_end:
        print("Successfully repaired main.c structure.")
        with open('main.c', 'w') as f:
            f.writelines(new_lines)
    else:
        print("Failed to find start/end markers.")
        print(f"Start: {found_start}, End: {found_end}")

if __name__ == "__main__":
    repair_main()
