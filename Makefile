# For this project we require that your code compiles
# cleanly (without warnings), hence the -Werror option
myshell: myshell.c
        gcc -Wall -Werror -o myshell myshell.c

clean:
