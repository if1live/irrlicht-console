#irrlicht-console

Quake like console for changing program variables or call function at runtime.

Rather than having to recompile your program every time you want to change a variable, 
or implement a GUI for all your variables, GLConsole's CVars allow you to change variables
on a simple command line while your program is running. (from [GLConsole][glconsole])

irrlicht-console = [GLConsole][glconsole] + [IrrConsole][irrconsole]

## Preview
![Basic](https://raw.github.com/if1live/irrlicht-console/master/documentation/basic.png)
![Help](https://raw.github.com/if1live/irrlicht-console/master/documentation/help.png)

## Key
* ~/` : Show console
* Left / Right : Move Cursor
* Home / Ctrl-a : Cursor to beginning of line
* End / Ctrl-e : Cursor to end of line
* Up : Display previous command in history on command line.
* Down : Go forward in the history.
* Shift + Up : Scroll up line
* Shift + Down : Scroll down line
* Page Up : Scroll up
* Page Down : Scroll down
* Tab : Auto complete

## Reference
* [GLConsole][glconsole]
* [IrrConsole][irrconsole]

[glconsole]: http://www.robots.ox.ac.uk/~gsibley/GLConsole/
[irrconsole]: http://www.oocities.org/standard_template/irrconsole/index.html
