/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#define _POSIX_C_SOURCE 200809L 
#define _XOPEN_SOURCE
#include <signal.h>
#include <bits/sigaction.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

// #ifndef SA_RESTART
// #define SA_RESTART 0 // Use a suitable default value or handle it appropriately.
// #endif           //

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);  //解释命令行输入
int builtin_cmd(char **argv);  //识别命令
void do_bgfg(char **argv);   //实现bg,fg
void waitfg(pid_t pid);    

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {  
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
pid_t Fork(){
    pid_t pid;
    if((pid = fork()) < 0){
        unix_error("fork error");
    }
    return pid;
}
//Fork() 系统函数要检查返回值，不能忘记。用包装函数使程序可读
void eval(char *cmdline) 
{
    char* argv[MAXARGS];
    char buf[MAXLINE];
    pid_t c_pid = 0; //创建的新进程 

    strcpy(buf,cmdline);
    int bg = parseline(buf,argv); 
    //查看是否后台运行,以及拆分cmdline

    sigset_t mask_all,mask_one,prev_one;
    sigfillset(&mask_all);
    sigemptyset(&mask_one);
    sigaddset(&mask_one,SIGCHLD);
    //用于阻塞

    if(!argv[0]){
        printf("blankinput (test)");
        return;
    }
    /////检测空输入
    if(!builtin_cmd(argv)){
        sigprocmask(SIG_BLOCK,&mask_one,&prev_one);
        if((c_pid = Fork()) == 0){
            c_pid = getpid();
            setpgid(0,0);
            //setpgid(0,0);  //修改pid，以防止sig杀掉终端
            addjob(jobs,c_pid,bg,buf);
            sigprocmask(SIG_SETMASK,&prev_one,NULL);
            if(execve(argv[0],argv,environ) < 0){
                deletejob(jobs,c_pid);                
                printf("%s : 妹找到请求的程序，寄了？\n",buf);
                exit(0);
            }
        }
        if(!bg){
            waitfg(c_pid);
            //deletejob(jobs,c_pid);
        }
        else{
            sigprocmask(SIG_BLOCK,&mask_one,&prev_one);
            printf("[%d] (%d) %s", pid2jid(c_pid), c_pid, cmdline);
          // printf("%d %s",c_pid,buf);
            
            sigprocmask(SIG_SETMASK,&prev_one,NULL);     ///////printf 也要阻塞 //容易抢锁
        }
        return;
    }

    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if(!strcmp(argv[0],"bg")){
        do_bgfg(argv);
        return 1;
        ////待处理
    }
    else if(!strcmp(argv[0],"fg")){
        do_bgfg(argv);
        return 1;
        //待处理
    }
    else if(!strcmp(argv[0],"jobs")){
        listjobs(jobs);
        return 1;
    }
    else if(!strcmp(argv[0],"quit")){
        printf("return by ""quit""");
        exit(0);
        return 1;
    }
    return 0;

}
/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    //////do_bgfg的问题有点大
    // int _pid_i = -1;
    // int _job_i = -1;
    // if (argv[1][0] == '%'){
    //     //jid "%"
    //     char * t = argv[1]+1;
    //     _job_i = atoi(t);
    // }
    // else if(strlen(argv[1]) == strspn(argv[1],"0123456789")){
    //     //pid
    //     _pid_i = atoi(argv[1]);
    // }
    // else{
    //     printf("wrong jobs input");
    //     return;
    // }

    // struct job_t* chan = NULL;
    // if(_pid_i == -1 && _job_i != -1){
    //         chan = getjobjid(jobs,_job_i);
    // }
    // else if(_pid_i != -1 && _job_i == -1){
    //         chan = getjobpid(jobs,_pid_i);
    // }
    // //pid/jid ->job
    // if(chan->state != ST)
    // {
    //     printf("job %d is not ST", _job_i);
    //     return;
    // }

    // if(!strcmp(argv[0],"bg")){
    //     //bg
    //     chan->state = BG;
    //     kill(-chan->pid,SIGCONT);
    //     ///
    //     //负数，以发给进程组
    //     return;
    // }
    // else if(!strcmp(argv[0],"fg")){
    //     //fg
    //     chan->state = FG;
    //     kill(-chan->pid,SIGCONT);
    //     waitfg(chan->pid);
    //     return;
    // }
    // else{
    //     printf("wrong 1");
    //     return;
    // }
    char* cmd = argv[0];
    char* id = argv[1];
    struct job_t* job;

    if (id == NULL) {
        printf("%s command requires PID or %%jobid argument\n", cmd);
        return;
    }

    // 根据 jid/pid 获取作业
    if (id[0] == '%') {
        if ((job = getjobjid(jobs, atoi(id + 1))) == NULL) {
            printf("%s: No such job\n", id);
            return;
        }
    } else if (atoi(id) > 0) {
        if ((job = getjobpid(jobs, atoi(id))) == NULL) {
            printf("(%d): No such process\n", atoi(id));
            return;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", cmd);
        return;
    }

    // 状态转移
    if (!strcmp(cmd, "fg")) {
        job->state = FG;
        kill(-job->pid, SIGCONT);
        waitfg(job->pid);
    } else if (!strcmp(cmd, "bg")) {
        job->state = BG;
        kill(-job->pid, SIGCONT);
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
} 

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    struct job_t* t = getjobpid(jobs,pid);
    if(t == NULL){
        return;
    }
    sigset_t musk;
    sigemptyset(&musk);
    while(fgpid(jobs) == pid){
        sigsuspend(&musk);
    }

}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int olderrno = errno;
    ///////在信号处理程序里要保原错误信息

    sigset_t mask_all,prev_all;
    int status;
    pid_t pid;

    sigfillset(&mask_all);


    while((pid = waitpid(-1,&status,WNOHANG|WUNTRACED)) > 0){
        if(WIFEXITED(status)){
            sigprocmask(SIGCHLD, &mask_all, &prev_all);
            deletejob(jobs, pid);
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
        else if(WIFSIGNALED(status)){
            sigprocmask(SIGCHLD, &mask_all, &prev_all);
            printf("Job [%d] (%d) terminated by signal 2\n", pid2jid(pid), pid);
            deletejob(jobs, pid);
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        } 
        else if(WIFSTOPPED(status)){
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

            struct job_t* t_job = getjobpid(jobs, pid);
            t_job->state = ST;
            printf("Job [%d] (%d) stopped by signal 20\n", t_job->jid, t_job->pid);

            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }

    }
    


    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int error = errno;
    pid_t foregroud;
    if((foregroud = fgpid(jobs)) <= 0){
        printf("there is no fg job\n");
    }
    else{
        kill(-foregroud,SIGINT);
        //deletejob(jobs,foregroud);
        //同理，跳转到sigchid了
    }
    errno = error;
   return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sendingl it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int error = errno;
    pid_t front;
    if((front = fgpid(jobs))== 0){
        printf("there if no fg");
    }
    else{
        kill(-front,SIGTSTP);
        ////deletejob(jobs,front);
        //这里会转到sigchld里delete
    }
    errno = error;
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}




