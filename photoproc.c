/*
** ATOP - System & Process Monitor 
** 
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-/thread-level.
** 
** This source-file contains functions to read the process-administration
** of every running process from kernel-space and extract the required
** activity-counters.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
** --------------------------------------------------------------------------
** Copyright (C) 2000-2010 Gerlof Langeveld
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or (at your option) any
** later version.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
** --------------------------------------------------------------------------
**
** $Log: photoproc.c,v $
** Revision 1.33  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.32  2009/11/27 13:44:00  gerlof
** euid, suid, fsuid, egid, sgid and fsgid also registered.
**
** Revision 1.31  2008/03/06 08:38:14  gerlof
** Register/show ppid of a process.
**
** Revision 1.30  2008/01/18 07:36:29  gerlof
** Gather information about the state of the individual threads.
**
** Revision 1.29  2007/11/05 12:26:10  gerlof
** Detect disappearing /proc/stat file  when process exits
** (credits: Rene Rebe).
**
** Revision 1.28  2007/03/27 10:53:59  gerlof
** Bug-solution: only allow IOSTAT when patches are not installed.
**
** Revision 1.27  2007/03/21 14:21:37  gerlof
** Handle io counters maintained from 2.6.20
**
** Revision 1.26  2007/02/13 10:32:34  gerlof
** Removal of external declarations.
**
** Revision 1.25  2007/01/15 09:00:14  gerlof
** Add new function to count actual number of processes.
**
** Revision 1.24  2006/02/07 06:47:35  gerlof
** Removed swap-counter.
**
** Revision 1.23  2005/10/21 09:49:57  gerlof
** Per-user accumulation of resource consumption.
**
** Revision 1.22  2004/12/14 15:05:58  gerlof
** Implementation of patch-recognition for disk and network-statistics.
**
** Revision 1.21  2004/09/23 09:07:49  gerlof
** Solved segmentation fault by checking tval.
**
** Revision 1.20  2004/09/08 06:01:01  gerlof
** Correct the priority of a process by adding 100 (the kernel
** subtracts 100 when showing the value via /proc).
**
** Revision 1.19  2004/09/02 10:49:45  gerlof
** Added sleep-average to process-info.
**
** Revision 1.18  2004/08/31 09:51:36  gerlof
** Gather information about underlying threads.
**
** Revision 1.17  2003/07/07 09:26:59  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.16  2003/06/30 11:30:43  gerlof
** Enlarge counters to 'long long'.
**
** Revision 1.15  2003/02/06 12:09:23  gerlof
** Exchange tab-character in command-line by space.
**
** Revision 1.14  2003/01/24 14:19:39  gerlof
** Exchange newline byte in command-line by space.
**
** Revision 1.13  2003/01/17 14:21:41  root
** Change-directory to /proc to optimize opening /proc-files
** via relative path-names i.s.o. absolute path-names.
**
** Revision 1.12  2003/01/17 07:31:29  gerlof
** Store the full command-line for every process.
**
** Revision 1.11  2003/01/06 13:03:09  gerlof
** Improved command-name parsing (command-names containing a close-bracket
** were not parsed correctly).
**
** Revision 1.10  2002/10/03 11:12:39  gerlof
** Modify (effective) uid/gid to real uid/gid.
**
** Revision 1.9  2002/07/24 11:13:31  gerlof
** Changed to ease porting to other UNIX-platforms.
**
** Revision 1.8  2002/07/08 09:27:45  gerlof
** Avoid buffer overflow during sprintf by using snprintf.
**
** Revision 1.7  2002/01/22 13:39:53  gerlof
** Support for number of cpu's.
**
** Revision 1.6  2001/11/22 08:33:43  gerlof
** Add priority per process.
**
** Revision 1.5  2001/11/13 08:26:15  gerlof
** Small bug-fixes.
**
** Revision 1.4  2001/11/07 09:18:43  gerlof
** Use /proc instead of /dev/kmem for process-level statistics.
**
** Revision 1.3  2001/10/04 13:57:34  gerlof
** Explicit include of sched.h (i.s.o. linux/sched.h via linux/mm.h).
**
** Revision 1.2  2001/10/04 08:47:26  gerlof
** Improved verification of kernel-symbol addresses
**
** Revision 1.1  2001/10/02 10:43:29  gerlof
** Initial revision
**
*/

#include <sys/types.h>
#include <sys/param.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "atop.h"
#include "photoproc.h"

#define	SCANSTAT 	"%c   %d   %*d  %*d  %*d %*d  "	\
			"%*d  %lld %*d  %lld %*d %lld "	\
			"%lld %*d  %*d  %d   %d  %*d  "	\
			"%*d  %ld  %lld %lld %*d %*d  "	\
			"%*d  %*d  %*d  %*d  %*d %*d  " \
			"%*d  %*d  %*d  %*d  %*d %*d  "	\
			"%d   %d   %d   %lld"

/* ATOP-extension line of /proc/pid/stat */
#define ATOPSTAT	"%lld %llu %lld %llu %lld %llu %lld %llu "	\
			"%lld %llu %lld %llu %lld %lld"

static int	procstat(struct tstat *, unsigned long long, char);
static int	procstatus(struct tstat *);
static int	procio(struct tstat *);
static int	proccont(struct tstat *);
static void	proccmd(struct tstat *);
static void	procsmaps(struct tstat *);
static void	procwchan(struct tstat *);
static count_t	procschedstat(struct tstat *);

unsigned long
photoproc(struct tstat *tasklist, int maxtask)
{
	static int			firstcall = 1;
	static unsigned long long	bootepoch;

	register struct tstat	*curtask;

	FILE		*fp;
	DIR		*dirp;
	struct dirent	*entp;
	char		origdir[1024], dockstat=0;
	unsigned long	tval=0;

	/*
	** one-time initialization stuff
	*/
	if (firstcall)
	{
		/*
		** check if this kernel offers io-statistics per task
		*/
		regainrootprivs();

		if ( (fp = fopen("/proc/1/io", "r")) )
		{
			supportflags |= IOSTAT;
			fclose(fp);
		}

		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");

		/*
 		** find epoch time of boot moment
		*/
		bootepoch = getboot();

		firstcall = 0;
	}

	/*
	** probe if the netatop module and (optionally) the
	** netatopd daemon are active
	*/
	regainrootprivs();

	netatop_probe();

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	/*
	** read all subdirectory-names below the /proc directory
	*/
	if ( getcwd(origdir, sizeof origdir) == NULL)
		mcleanstop(53, "failed to save current dir\n");

	if ( chdir("/proc") == -1)
		mcleanstop(54, "failed to change to /proc\n");

	dirp = opendir(".");

	while ( (entp = readdir(dirp)) && tval < maxtask )
	{
		/*
		** skip non-numerical names
		*/
		if (!isdigit(entp->d_name[0]))
			continue;

		/*
		** change to the process' subdirectory
		*/
		if ( chdir(entp->d_name) != 0 )
			continue;

		/*
 		** gather process-level information
		*/
		curtask	= tasklist+tval;

		if ( !procstat(curtask, bootepoch, 1)) /* from /proc/pid/stat */
		{
			if ( chdir("..") == -1);
			continue;
		}

		if ( !procstatus(curtask) )	/* from /proc/pid/status  */
		{
			if ( chdir("..") == -1);
			continue;
		}

		if ( !procio(curtask) )		/* from /proc/pid/io      */
		{
			if ( chdir("..") == -1);
			continue;
		}

		procschedstat(curtask);		/* from /proc/pid/schedstat */
		proccmd(curtask);		/* from /proc/pid/cmdline */
		dockstat += proccont(curtask);	/* from /proc/pid/cpuset  */

		/*
		** reading the smaps file for every process with every sample
		** is a really 'expensive' from a CPU consumption point-of-view,
		** so gathering this info is optional
		*/
		if (calcpss)
			procsmaps(curtask);	/* from /proc/pid/smaps */

		/*
		** determine thread's wchan, if wanted ('expensive' from
		** a CPU consumption point-of-view)
		*/
                if (getwchan)
                	procwchan(curtask);

		// read network stats from netatop
		netatop_gettask(curtask->gen.tgid, 'g', curtask);

		tval++;		/* increment for process-level info */

		/*
 		** if needed (when number of threads is larger than 1):
		**   read and fill new entries with thread-level info
		*/
		if (curtask->gen.nthr > 1)
		{
			DIR		*dirtask;
			struct dirent	*tent;

			curtask->gen.nthrrun  = 0;
			curtask->gen.nthrslpi = 0;
			curtask->gen.nthrslpu = 0;

			/*
 			** rundelay and blkdelay on process level only
			** concerns the delays of the main thread;
			** totalize the delays of all threads
			*/
			curtask->cpu.rundelay = 0;
			curtask->cpu.blkdelay = 0;
			
			/*
			** open underlying task directory
			*/
			if ( chdir("task") == 0 )
			{
				unsigned long cur_nth = 0;

				dirtask = opendir(".");
	
				/*
				** due to race condition, opendir() might
				** have failed (leave task and process-level
				** directories)
				*/
				if( dirtask == NULL )		
				{
					if(chdir("../..") == -1); 
					continue;
				}

				while ((tent=readdir(dirtask)) && tval<maxtask)
				{
					struct tstat *curthr = tasklist+tval;

					/*
					** change to the thread's subdirectory
					*/
					if ( tent->d_name[0] == '.'  ||
					     chdir(tent->d_name) != 0 )
						continue;

					if ( !procstat(curthr, bootepoch, 0))
					{
						if ( chdir("..") == -1);
						continue;
					}
			
					if ( !procstatus(curthr) )
					{
						if ( chdir("..") == -1);
						continue;
					}

					if ( !procio(curthr) )
					{
						if ( chdir("..") == -1);
						continue;
					}

					/*
					** determine thread's wchan, if wanted
 					** ('expensive' from a CPU consumption
					** point-of-view)
					*/
                			if (getwchan)
                        			procwchan(curthr);

					// totalize delays of all threads
					curtask->cpu.rundelay +=
						procschedstat(curthr);

					curtask->cpu.blkdelay +=
						curthr->cpu.blkdelay;

					strcpy(curthr->gen.container,
						curtask->gen.container);

					switch (curthr->gen.state)
					{
	   		   		   case 'R':
						curtask->gen.nthrrun  += 1;
						break;
	   		   		   case 'S':
						curtask->gen.nthrslpi += 1;
						break;
	   		   		   case 'I':
	   		   		   case 'D':
						curtask->gen.nthrslpu += 1;
						break;
					}

					curthr->gen.nthr = 1;

					// read network stats from netatop
					netatop_gettask(curthr->gen.pid, 't',
								curthr);

					// all stats read now
					tval++;	    /* increment thread-level */
					cur_nth++;  /* increment # threads    */

					if ( chdir("..") == -1); /* thread */
				}

				closedir(dirtask);
				if ( chdir("..") == -1); /* leave task */

				// calibrate number of threads
				curtask->gen.nthr = cur_nth;
			}
		}

		if ( chdir("..") == -1); /* leave process-level directry */
	}

	closedir(dirp);

	if ( chdir(origdir) == -1)
		mcleanstop(55, "cannot change to %s\n", origdir);

	if (dockstat)
		supportflags |= DOCKSTAT;
	else
		supportflags &= ~DOCKSTAT;

	return tval;
}

/*
** count number of tasks in the system, i.e.
** the number of processes plus the total number of threads
*/
unsigned long
counttasks(void)
{
	unsigned long	nr=0;
	char		linebuf[256];
	FILE		*fp;
	DIR             *dirp;
	struct dirent   *entp;
	char            origdir[1024];

	/*
	** determine total number of threads 
	*/
	if ( (fp = fopen("/proc/loadavg", "r")) != NULL)
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if ( sscanf(linebuf, "%*f %*f %*f %*d/%lu", &nr) < 1)
				mcleanstop(53, "wrong /proc/loadavg\n");
		}
		else
			mcleanstop(53, "unreadable /proc/loadavg\n");

		fclose(fp);
	}
	else
		mcleanstop(53, "can not open /proc/loadavg\n");


	/*
	** add total number of processes 
	*/
	if ( getcwd(origdir, sizeof origdir) == NULL)
		mcleanstop(53, "cannot determine cwd\n");

	if ( chdir("/proc") == -1)
		mcleanstop(53, "cannot change to /proc\n");

	dirp = opendir(".");

	while ( (entp = readdir(dirp)) )
	{
		/*
		** count subdirectory names under /proc starting with a digit
		*/
		if (isdigit(entp->d_name[0]))
			nr++;
	}

	closedir(dirp);

	if ( chdir(origdir) == -1)
		mcleanstop(53, "cannot change to %s\n", origdir);

	return nr;
}

/*
** open file "stat" and obtain required info
*/
static int
procstat(struct tstat *curtask, unsigned long long bootepoch, char isproc)
{
	FILE	*fp;
	int	nr;
	char	line[4096], *p, *cmdhead, *cmdtail;

	if ( (fp = fopen("stat", "r")) == NULL)
		return 0;

	if ( (nr = fread(line, 1, sizeof line-1, fp)) == 0)
	{
		fclose(fp);
		return 0;
	}

	line[nr] = '\0';	// terminate string

	/*
    	** fetch command name
	*/
	cmdhead = strchr (line, '(');
	cmdtail = strrchr(line, ')');

	if (!cmdhead || !cmdtail || cmdtail < cmdhead) // parsing failed?
	{
		fclose(fp);
		return 0;
	}

	if ( (nr = cmdtail-cmdhead-1) > PNAMLEN)
		nr = PNAMLEN;

	p = curtask->gen.name;

	memcpy(p, cmdhead+1, nr);
	*(p+nr) = 0;

	while ( (p = strchr(p, '\n')) != NULL)
	{
		*p = '?';
		p++;
	}

	/*
  	** fetch other values
  	*/
	curtask->gen.isproc  = isproc;
	curtask->cpu.rtprio  = 0;
	curtask->cpu.policy  = 0;
	curtask->gen.excode  = 0;

	sscanf(line, "%d", &(curtask->gen.pid));  /* fetch pid */

	nr = sscanf(cmdtail+2, SCANSTAT,
		&(curtask->gen.state), 	&(curtask->gen.ppid),
		&(curtask->mem.minflt),	&(curtask->mem.majflt),
		&(curtask->cpu.utime),	&(curtask->cpu.stime),
		&(curtask->cpu.prio),	&(curtask->cpu.nice),
		&(curtask->gen.btime),
		&(curtask->mem.vmem),	&(curtask->mem.rmem),
		&(curtask->cpu.curcpu),	&(curtask->cpu.rtprio),
		&(curtask->cpu.policy), &(curtask->cpu.blkdelay));

	if (nr < 12)		/* parsing failed? */
	{
		fclose(fp);
		return 0;
	}

	/*
 	** normalization
	*/
	curtask->gen.btime   = (curtask->gen.btime+bootepoch)/hertz;
	curtask->cpu.prio   += 100; 	/* was subtracted by kernel */
	curtask->mem.vmem   /= 1024;
	curtask->mem.rmem   *= pagesize/1024;

	fclose(fp);

	switch (curtask->gen.state)
	{
  	   case 'R':
		curtask->gen.nthrrun  = 1;
		break;
  	   case 'S':
		curtask->gen.nthrslpi = 1;
		break;
	   case 'I':
  	   case 'D':
		curtask->gen.nthrslpu = 1;
		break;
	}

	return 1;
}

/*
** open file "status" and obtain required info
*/
static int
procstatus(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];

	if ( (fp = fopen("status", "r")) == NULL)
		return 0;

	curtask->gen.nthr     = 1;	/* for compat with 2.4 */
	curtask->cpu.sleepavg = 0;	/* for compat with 2.4 */
	curtask->mem.vgrow    = 0;	/* calculated later */
	curtask->mem.rgrow    = 0;	/* calculated later */

	while (fgets(line, sizeof line, fp))
	{
		if (memcmp(line, "Tgid:", 5) ==0)
		{
			sscanf(line, "Tgid: %d", &(curtask->gen.tgid));
			continue;
		}

		if (memcmp(line, "Pid:", 4) ==0)
		{
			sscanf(line, "Pid: %d", &(curtask->gen.pid));
			continue;
		}

		if (memcmp(line, "SleepAVG:", 9)==0)
		{
			sscanf(line, "SleepAVG: %d%%",
				&(curtask->cpu.sleepavg));
			continue;
		}

		if (memcmp(line, "Uid:", 4)==0)
		{
			sscanf(line, "Uid: %d %d %d %d",
				&(curtask->gen.ruid), &(curtask->gen.euid),
				&(curtask->gen.suid), &(curtask->gen.fsuid));
			continue;
		}

		if (memcmp(line, "Gid:", 4)==0)
		{
			sscanf(line, "Gid: %d %d %d %d",
				&(curtask->gen.rgid), &(curtask->gen.egid),
				&(curtask->gen.sgid), &(curtask->gen.fsgid));
			continue;
		}

		if (memcmp(line, "envID:", 6) ==0)
		{
			sscanf(line, "envID: %d", &(curtask->gen.ctid));
			continue;
		}

		if (memcmp(line, "VPid:", 5) ==0)
		{
			sscanf(line, "VPid: %d", &(curtask->gen.vpid));
			continue;
		}

		if (memcmp(line, "Threads:", 8)==0)
		{
			sscanf(line, "Threads: %d", &(curtask->gen.nthr));
			continue;
		}

		if (memcmp(line, "VmData:", 7)==0)
		{
			sscanf(line, "VmData: %lld", &(curtask->mem.vdata));
			continue;
		}

		if (memcmp(line, "VmStk:", 6)==0)
		{
			sscanf(line, "VmStk: %lld", &(curtask->mem.vstack));
			continue;
		}

		if (memcmp(line, "VmExe:", 6)==0)
		{
			sscanf(line, "VmExe: %lld", &(curtask->mem.vexec));
			continue;
		}

		if (memcmp(line, "VmLib:", 6)==0)
		{
			sscanf(line, "VmLib: %lld", &(curtask->mem.vlibs));
			continue;
		}

		if (memcmp(line, "VmSwap:", 7)==0)
		{
			sscanf(line, "VmSwap: %lld", &(curtask->mem.vswap));
			continue;
		}

		if (memcmp(line, "VmLck:", 6)==0)
		{
			sscanf(line, "VmLck: %lld", &(curtask->mem.vlock));
			continue;
		}

		if (memcmp(line, "SigQ:", 5)==0)
			break;
	}

	fclose(fp);
	return 1;
}

/*
** open file "io" (>= 2.6.20) and obtain required info
*/
#define	IO_READ		"read_bytes:"
#define	IO_WRITE	"write_bytes:"
#define	IO_CWRITE	"cancelled_write_bytes:"
static int
procio(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];
	count_t	dskrsz=0, dskwsz=0, dskcwsz=0;

	if (supportflags & IOSTAT)
	{
		regainrootprivs();

		if ( (fp = fopen("io", "r")) )
		{
			while (fgets(line, sizeof line, fp))
			{
				if (memcmp(line, IO_READ,
						sizeof IO_READ -1) == 0)
				{
					sscanf(line, "%*s %llu", &dskrsz);
					dskrsz /= 512;		// in sectors
					continue;
				}

				if (memcmp(line, IO_WRITE,
						sizeof IO_WRITE -1) == 0)
				{
					sscanf(line, "%*s %llu", &dskwsz);
					dskwsz /= 512;		// in sectors
					continue;
				}

				if (memcmp(line, IO_CWRITE,
						sizeof IO_CWRITE -1) == 0)
				{
					sscanf(line, "%*s %llu", &dskcwsz);
					dskcwsz /= 512;		// in sectors
					continue;
				}
			}

			fclose(fp);

			curtask->dsk.rsz	= dskrsz;
			curtask->dsk.rio	= dskrsz;  // to enable sort
			curtask->dsk.wsz	= dskwsz;
			curtask->dsk.wio	= dskwsz;  // to enable sort
			curtask->dsk.cwsz	= dskcwsz;
		}

		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");
	}

	return 1;
}

/*
** store the full command line; the command-line may contain:
**    - null-bytes as a separator between the arguments
**    - newlines (e.g. arguments for awk or sed)
**    - tabs (e.g. arguments for awk or sed)
** these special bytes will be converted to spaces
*/
static void
proccmd(struct tstat *curtask)
{
	FILE		*fp;
	register int 	i, nr;

	memset(curtask->gen.cmdline, 0, CMDLEN+1);

	if ( (fp = fopen("cmdline", "r")) != NULL)
	{
		register char *p = curtask->gen.cmdline;

		nr = fread(p, 1, CMDLEN, fp);
		fclose(fp);

		if (nr >= 0)	/* anything read ? */
		{
			for (i=0; i < nr-1; i++, p++)
			{
				switch (*p)
				{
				   case '\0':
				   case '\n':
				   case '\t':
					*p = ' ';
				}
			}
		}
	}
}


/*
** determine the wait channel of a sleeping thread
** i.e. the name of the kernel function in which the thread
** has been put in sleep state)
*/
static void
procwchan(struct tstat *curtask)
{
        FILE            *fp;
        register int    nr = 0;

        if ( (fp = fopen("wchan", "r")) != NULL)
        {

                nr = fread(curtask->cpu.wchan, 1,
			sizeof(curtask->cpu.wchan)-1, fp);
                if (nr < 0)
                        nr = 0;
                fclose(fp);
        }

        curtask->cpu.wchan[nr] = 0;
}


/*
** store the container ID, retrieved from the 'cpuset'
** that might look like this:
**
** In case of Docker:
**    /system.slice/docker-af78216c2a230f1aa5dce56cbf[SNAP].scope (e.g. CentOS)
**    /docker/af78216c2a230f1aa5dce56cbf[SNAP]   (e.g. openSUSE and Ubuntu))
**
** In case of Docker created by K8s:
**    /kubepods/burstable/pod07dbb922-[SNAP]/223dc5e15b[SNAP]
**
** In case of podman:
**    /machine.slice/libpod-0b5836e9ea98aefd89481123bi[SNAP].scope
**
** In general:
** - search for last '/' (basename)
** - check if '/' followed by 'docker-': then skip 'docker-'
** - check if '/' followed by 'libpod-': then skip 'libpod-'
** - take 12 positions for the container ID
**
** Return value:
**	0 - no container
**	1 - container
*/
#define	CIDSIZE		12
#define	SHA256SIZE	64
#define	DOCKPREFIX	"docker-"
#define	PODMANPREFIX	"libpod-"

static int
proccont(struct tstat *curtask)
{
	FILE	*fp;
	char	line[256];

	if ( (fp = fopen("cpuset", "r")) != NULL)
	{
		register char *p;

		if ( fgets(line, sizeof line, fp) )
		{
			fclose(fp);

			// fast check for processes not using cpuset
			// i.e. anyhow not container
			if (memcmp(line, "/\n", 3) == 0)
				return 0;

			// possibly container: find basename in path and
			// verify that its minimum length is the size of SHA256
			if ( (p = strrchr(line, '/')) != NULL &&
			                    strlen(p) >= SHA256SIZE)
			{
				p++;

				if (memcmp(p, DOCKPREFIX,
						sizeof(DOCKPREFIX)-1) == 0)
				{
					p += sizeof(DOCKPREFIX)-1;
				}
				else
				{
					if (memcmp(p, PODMANPREFIX,
						sizeof(PODMANPREFIX)-1) == 0)
					{
						p += sizeof(PODMANPREFIX)-1;
					}
				}

				memcpy(curtask->gen.container, p, CIDSIZE);
				return 1;
			}
		}
		else
		{
			fclose(fp);
		}
	}

	return 0;
}


/*
** open file "smaps" and obtain required info
** since Linux-4.14, kernel supports "smaps_rollup" which has better
** performence. check "smaps_rollup" in first call
** if kernel supports "smaps_rollup", use "smaps_rollup" instead
*/
static void
procsmaps(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];
	count_t	pssval;
	static int procsmaps_firstcall = 1;
	static char *smapsfile = "smaps";

	if (procsmaps_firstcall)
	{
		regainrootprivs();
		if ( (fp = fopen("/proc/1/smaps_rollup", "r")) )
		{
			smapsfile = "smaps_rollup";
			fclose(fp);
		}

		procsmaps_firstcall = 0;
	}

	/*
 	** open the file (always succeeds, even if no root privs)
	*/
	regainrootprivs();

	if ( (fp = fopen(smapsfile, "r")) )
	{
		curtask->mem.pmem = 0;

		while (fgets(line, sizeof line, fp))
		{
			if (memcmp(line, "Pss:", 4) != 0)
				continue;

			// PSS line found to be accumulated
			sscanf(line, "Pss: %llu", &pssval);
			curtask->mem.pmem += pssval;
		}

		/*
		** verify if fgets returned NULL due to error i.s.o. EOF
		*/
		if (ferror(fp))
			curtask->mem.pmem = (unsigned long long)-1LL;

		fclose(fp);
	}
	else
	{
		curtask->mem.pmem = (unsigned long long)-1LL;
	}

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");
}

/*
** get run_delay from /proc/<pid>/schedstat
** ref: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/scheduler/sched-stats.rst?h=v5.7-rc6
*/
static count_t
procschedstat(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];
	count_t	runtime, rundelay = 0;
	unsigned long pcount;
	static char *schedstatfile = "schedstat";

	/*
 	** open the schedstat file 
	*/
	if ( (fp = fopen(schedstatfile, "r")) )
	{
		curtask->cpu.rundelay = 0;

		if (fgets(line, sizeof line, fp))
		{
			sscanf(line, "%llu %llu %lu\n",
					&runtime, &rundelay, &pcount);

			curtask->cpu.rundelay = rundelay;
		}

		/*
		** verify if fgets returned NULL due to error i.s.o. EOF
		*/
		if (ferror(fp))
			curtask->cpu.rundelay = 0;

		fclose(fp);
	}
	else
	{
		curtask->cpu.rundelay = 0;
	}

	return curtask->cpu.rundelay;
}
