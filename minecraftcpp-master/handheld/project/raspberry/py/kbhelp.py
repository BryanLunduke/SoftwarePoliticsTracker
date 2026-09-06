try:
    import msvcrt

    """Win32 implementation of a keyboard input reader"""
    class KeyboardInput_win32:
        def __init__(self):
            self.setBlocking(True)

        def clear(self):
            while 1:
                if self.readChar_nonblocking() is None:
                    break

        def setBlocking(self, block):
            self.readChar = self.readChar_blocking if block else \
                            self.readChar_nonblocking

        def hasChar(self):
            return msvcrt.kbhit()

        def readChar_blocking(self):
            return msvcrt.getch()

        def readChar_nonblocking(self):
            if not msvcrt.kbhit(): return None
            return msvcrt.getch()

    KeyboardInput = KeyboardInput_win32

except:
    try:
        class KeyboardInput_unix:
            def hasChar(self):
                import sys, tty, termios
                from select import select
                fd = sys.stdin.fileno()
                old_settings = termios.tcgetattr(fd)
                try:
                    tty.setraw(sys.stdin.fileno())
                    [i, o, e] = select([sys.stdin.fileno()], [], [], 5)
                    if i: ch = True
                    else: ch = False
                finally:
                    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
                return ch

            def readChar(self):
                import sys, tty, termios
                from select import select
                fd = sys.stdin.fileno()
                old_settings = termios.tcgetattr(fd)
                try:
                    tty.setraw(sys.stdin.fileno())
                    ch = sys.stdin.read(1)
                finally:
                    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
                return ch

        KeyboardInput = KeyboardInput_unix
    except: pass
