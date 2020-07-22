* tested with valgrind and there are no memory leaks but
    there is an error for reading invalid size 1 when i check for !\n.

* when running the program with something like ./s-talk 5000 localhost 5001 >> out.txt
    the program finishes writing to the file, but in the end it seg faults for some reason
    when it reaches pthread cancel trying to shut down the threads. 
    
    when running that command without >> and printing to terminal, it works fine...

* for undefined cases such as getting an error binding, max nodes reached, I exit the program.

