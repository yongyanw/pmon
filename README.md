# pmon
pmon is a computer performance system monitor tool which can capture process information for a period time and save it to a file. It's a tool based on nmon having all the functionalities of nmon and an additonal functionality with atop to capture specified process information and save it to a file which can be shown as charts in NMONVisualizer.

- nmon is a computer performance system monitor tool(see https://nmon.sourceforge.net/pmwiki.php)

- atop, which is an ASCII full-screen performance monitor for Linux that is capable of reporting the activity of all processes(see https://github.com/Atoptool/atop), is integrated in pmon to support capturing disk I/O and network information

# Additional functionality
Multiple processes can be specified to capture their resources usages, including CPU, memory, disk I/O and network. 

The root privilege is needed to capture the disk I/O information, for the network information, netatop service should be started before running pmon.

Please check https://www.atoptool.nl/downloadnetatop.php for netatop installation.

# Option to specify the processes
The option to specify the processes is:

-i pid1,pname1[,pid2,pname2][,pid3,pname3]...

Multiple processes can be specified. The process name can be anything you want which is just used to show the metrics in NMONVisualizer. The metrics/resources usages will be shown as pname.cpu, pname.mem, pname.disk and pname.net.

# Example to capture process information
The following is an example to capture the process information for both the processes with pid=26165 and pid=287:

./pmon -f -c 100 -s 1 -F ./example.nmon -i 26165,node1,287,node2


