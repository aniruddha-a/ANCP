/*
 *  IOS like CLI completion - Using uncompressed trie on ASCII lowercase
 *  Alphabet. This only completes known commands and does not read input
 *  from within the command 
 *
 *  Tue Sep 29 16:09:15 IST 2009
 *  Aniruddha. A (aniruddha.a@gmail.com)
 */
#include <termios.h>

/*
 * @getch - shamelessly copied from  http://stackoverflow.com
 *          making this return int as we want to detect control chars
 */
int getch(void) {
    int buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    /*ani*/
    old.c_iflag |= ISTRIP;
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, sizeof (int)) < 0)
        perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return buf;
}
