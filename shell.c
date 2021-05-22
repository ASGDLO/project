#include "header.h"

struct COMMAND{ // 커맨드 구조체
        char* name;
        char* desc;
        bool ( *func )( int argc, char* argv[] ); // 함수포인터. 사용할 함수들의 매개변수를 맞춰줌
};

pid_t child=-1;                                            // 자식프로세스 pid 저장 전역변수
int status;                                                  // 프로세스 status
int list_CHLD[100];                                     // SIGTSTP로 실행을 멈춘 자식프로세스 목록
int count=0;                                                // waitpid 함수에 사용될 파라미터

myProc procList[PROCESS_MAX];	//완성한 myProc의 포인터 저장 배열

int procCnt = 0;				//현재까지 완성한 myProc 갯수

unsigned long uptime;			//os 부팅 후 지난 시간
unsigned long beforeUptime;		//이전 실행 시의 os 부팅 후 지난 시각
unsigned long memTotal;			//전체 물리 메모리 크기
unsigned int hertz;	 			//os의 hertz값 얻기(초당 context switching 횟수)

time_t now;
time_t before;

pid_t myPid;					//자기 자신의 pid
uid_t myUid;					//자기 자신의 uid
char myPath[PATH_LEN];			//자기 자신의 path
char myTTY[TTY_LEN];			//자기 자신의 tty

bool aOption = false;			//a Option	stat, command 활성화
bool uOption = false;			//u Option	user, cpu, mem, vsz, rss, start, command 활성화
bool xOption = false;			//x Option	stat, command 활성화

bool cmd_ls( int argc, char* argv[] );           // ls 명령어
bool cmd_cd( int argc, char* argv[] );          //cd 명령어
bool cmd_exit( int argc, char* argv[] );        //exit, quit 명령어
bool cmd_help( int argc, char* argv[] );       //help 명령어
bool cmd_ps(int argc, char* argv[]);            //lsb 명령어
bool cmd_cat(int argc, char* argv[]);            //cat 명령어
bool cmd_cp(int argc, char* argv[]);            //cp 명령어
void handler(int sig);                                   //signal handler


// 명령어 구조체
struct COMMAND  builtin_cmds[] =
{
    { "ls",     "print present directory file list",        cmd_ls   },
    { "cd",    "change directory",                    cmd_cd   },
    { "exit",   "exit this shell",                        cmd_exit  },
    { "quit",   "quit this shell",                        cmd_exit  },
    { "help",  "show this help",                      cmd_help },
    { "?",      "show this help",                      cmd_help },
    { "lsb",    "show process list" ,                 cmd_ps   },
    { "show",    "IO catenate",                     cmd_cat},
    { "copy",    "file copy and covered",           cmd_cp}
};


// ls 명령어 옵션 처리 함수
void ls_Inode(struct stat buf)
{
    printf("%d        ",(unsigned int)buf.st_ino);
}

void ls_Mode(struct stat buf)
{
    printf("%o        ",(unsigned int)buf.st_mode);
}

void ls_FSize(struct stat buf)
{
    printf("%d        ",(unsigned int)buf.st_size);
}

void ls_option(struct stat buf, char* option)
{
     if(strcmp(option, "-l") ==0)
     {
         ls_Mode(buf);
         ls_Inode(buf);
         ls_FSize(buf);
     }
     else if(strcmp(option, "-i")==0)
     {
         ls_Inode(buf);
     }
}    

void f_err(int erno)
{
    switch(erno) {
        case ENOENT : printf("해당 파일이 존재하지 않습니다.\n"); exit(0);
        case EACCES : printf("접근이 허용된 파일이 아닙니다.\n"); exit(0);
        case EROFS : printf("읽기전용 파일입니다.\n"); exit(0);
        default : printf("알수 없는 오류입니다.\n"); break;
    }
}
 
void writefile(char *in_f, char *out_f) //파일 복사 함수
{
    int in_o, out_o;
    int read_o;
 
    char buf[1024];
 
    in_o = open(in_f, O_RDONLY);
    out_o = open(out_f, O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
 
    while((read_o = read(in_o,buf,sizeof(buf))) > 0)
        write(out_o,buf,read_o);
}
 
void fileexception(int argcn, char *arg1, char *arg2) //예외처리 함수
{
    if(argcn < 4) { printf("대상파일이 입력되지 않았습니다.\n"); exit(0); }
    if(access(arg1,F_OK) != 0) { printf("원본 파일이 존재하지 않습니다.\n"); exit(0); }
    if(strcmp(arg1,arg2) == 0) { printf("원본과 대상이 같습니다.\n"); exit(0); }
}

bool cmd_ls( int argc, char* argv[] ){
   char* cwd=(char*)malloc(sizeof(char)* 1024);
   memset(cwd,0,1024);
   
   DIR *dir = NULL;
   struct dirent* entry;
   struct stat buf;
   
   getcwd(cwd,1024);
   
   if((dir=opendir(cwd)) == NULL)     // 목록을 읽을 디렉토리명으로 DIR을 리턴
   {
        printf("opendir() error\n");
        exit(1);
   }
   
   while((entry = readdir(dir))!=NULL){  // 디렉토리의 처음부터 파일 또는 디렉토리명을 순서대로 한개씩 읽기
        lstat(entry->d_name, &buf);   // 엔트리에 있는 디렉토리 및 파일 이름을 버퍼에 추가
        if(S_ISREG(buf.st_mode))
             printf("FILE  ");
        else if(S_ISDIR(buf.st_mode))
             printf("DIR   ");
        else
             printf("???   ");
        
        // 옵션이 있는 경우
        if(argc>1)
             ls_option(buf,argv[1]);
        printf("%s     \n",entry->d_name);
   }
   
   free(cwd);
   closedir(dir);    // 디렉토리 정보 close

   return true;
}

bool cmd_cd( int argc, char* argv[] ){ //cd : change directory
        if( argc == 1 )
                chdir( getenv( "HOME" ) );
        else if( argc == 2 ){
            if( chdir( argv[1] ) )
                printf( "No directory\n" );
        }
        else
            printf( "USAGE: cd [dir]\n" );
        
        return true;
}

bool cmd_exit( int argc, char* argv[] ){
        return false;
}

bool cmd_help( int argc, char* argv[] ){ // 명령어 출력
        int i;
        for( i = 0; i < sizeof( builtin_cmds ) / sizeof( struct COMMAND ); i++ )
        {
                if( argc == 1 || strcmp( builtin_cmds[i].name, argv[1] ) == 0 )
                        printf( "%-10s: %s\n", builtin_cmds[i].name, builtin_cmds[i].desc );
        }
        
        return true;
}

bool cmd_ps(int argc, char* argv[]){ //프로세스 목록 출력

        // 명령어 실행시마다 옵션 초기화
        aOption = false;
        uOption = false;
        xOption = false;

        // proc 디렉토리 비움
        erase_proc_list();
        
        memTotal = get_mem_total();					//전체 물리 메모리 크기
	hertz = (unsigned int)sysconf(_SC_CLK_TCK);	//os의 hertz값 얻기(초당 context switching 횟수)

	myPid = getpid();			//자기 자신의 pid

	char pidPath[FNAME_LEN];
	memset(pidPath, '\0', FNAME_LEN);
	sprintf(pidPath, "/%d", myPid);

	strcpy(myPath, PROC);			//자기 자신의 /proc 경로 획득
	strcat(myPath, pidPath);

	getTTY(myPath, myTTY);			//자기 자신의 tty 획득
	for(int i = strlen(PTS); i < strlen(myTTY); i++)
		if(!isdigit(myTTY[i])){
			myTTY[i] = '\0';
			break;
		}

	myUid = getuid();			//자기 자신의 uid

	for(int i = 1; i < argc; i++){					//Option 획득 - 옵션 중복 가능
		for(int j = 0; j < strlen(argv[i]); j++){
			switch(argv[i][j]){
				case 'a':
					aOption = true;
					break;
				case 'u':
					uOption = true;
					break;
				case 'x':
					xOption = true;
					break;
			}
		}
	}

	search_proc(true, aOption, uOption, xOption, NULL);

	print_pps();
	
	return true;
}

bool cmd_cat(int argc, char* argv[]){
        int i;
        
        for(i=1; i<argc; i++){
            FILE *f;
            int c;
            
            f = fopen(argv[i], "r");
            if(!f){
               perror(argv[i]);
               exit(1);
            }
            
            while((c=fgetc(f))!=EOF){
               if(putchar(c) < 0) exit(1);
            }
            fclose(f);
        }
        
        return true;
}

bool cmd_cp(int argc, char* argv[]){
    int opt;
    char bkname[64];
    char conin = 'y';
 
    if((opt = getopt(argc, argv, "b:f")) != -1) {
        switch(opt) {
            case 'b' :
                fileexception(argc, argv[2], argv[3]);
                if(argc > 3) {
                    if(access(argv[3],F_OK) == 0) {
                        strcpy(bkname,argv[3]);
                        strcat(bkname, "~");
                        rename(argv[3], bkname);
                    }
                    writefile(argv[2],argv[3]);
                }
                break;
            case 'f' :
                fileexception(argc, argv[2], argv[3]);
 
                if(access(argv[3],F_OK) == 0) {
                    if(unlink(argv[3])) { f_err(errno); }
                }
                writefile(argv[2],argv[3]);
                break;
            case '?' :
                printf("알수없는 옵션입니다."); 
                break;
        }
    }
    else {
        if(access(argv[1],F_OK) != 0) {
            printf("원본 파일이 존재하지 않습니다.\n");
            return true;
        }
 
        if(argc<3) {
            printf("대상 파일이 입력되지 않았습니다.\n");
            return true;
        }
 
        if(access(argv[2],F_OK) == 0) {
            while(conin == 'y' || conin == 'n') {
                printf("대상 파일이 이미 존재합니다. 덮어쓰시겠습니까?(y/n) ");
                conin = getchar();
 
                if(conin == 'y') {
                    if(unlink(argv[2])) {
                        f_err(errno);
                    }
                    writefile(argv[1],argv[2]);
                    conin = 'h';
                }
                else if(conin == 'n') {
                    printf("복사를 중단합니다.\n");
                    conin = 'h';
                    return true;
                }
            }
        }
        else {
            writefile(argv[1],argv[2]);
        }
    }
 
    return true;
}

void handler(int sig){             //signal 핸들러
        if(child != 0){                //자식과 부모의 구분
                switch(sig){
                    case SIGINT:
                            printf("Ctrl + c SIGINT\n");
                            break;
                    case SIGTSTP:
                            printf("Ctrl + z SIGTSTP\n");
                            kill(0,SIGCHLD); // stpt 받았을 때, 자신은 다시 run
                            break;
                    case SIGCONT:
                            printf("Restart rs SIGCONT\n");
                            break;
                }
        }
}

int tokenize( char* buf, char* delims, char* tokens[], int maxTokens ){
        int token_count = 0;
        char* token;
        token = strtok( buf, delims );
        while( token != NULL && token_count < maxTokens ){
                tokens[token_count] = token;
                token_count++;
                token = strtok( NULL, delims );
        }
        tokens[token_count] = NULL;
        return token_count;
}

bool run( char* line ){
        char delims[] = " \r\n\t";
        char* tokens[128];
        int token_count;
        int i;
        bool back;
        for(i=0;i<strlen(line);i++){ //background 실행은 wait하지 않는다.
                if(line[i] == '&'){
                        back=true;
                        line[i]='\0';
                        break;
                }
        }
        
        // 입력받은 명령어 문자열 토큰화
        token_count = tokenize( line, delims, tokens, sizeof( tokens ) / sizeof( char* ) );
        if( token_count == 0 )
                return true;
        // cmd 명령어 구조체에서 이름에 맞는 해당 명령어를 찾아 함수를 반환
        for( i = 0; i < sizeof( builtin_cmds ) / sizeof( struct COMMAND ); i++ ){     
                if( strcmp( builtin_cmds[i].name, tokens[0] ) == 0 )
                        return builtin_cmds[i].func( token_count, tokens );
        }

        child = fork();
        if( child == 0 ){
                if( signal(SIGINT,handler) == SIG_ERR){     // 시그널 핸들러 처리
                        printf("SIGINT Error\n");
                        _exit(1);
                }
                execvp( tokens[0], tokens );
                printf( "No such file\n" );
                _exit( 0 );
        }
        else if( child < 0 )
        {
            printf( "Failed to fork()!" );
            _exit( 0 );
        }
        else if(back == false){
            waitpid(child, &status, WUNTRACED );
        }
        return true;
}

int main(){
        char line[1024];
        signal(SIGINT,handler);
        signal(SIGTSTP,handler);
        while(1)
        {
            printf( "[%s] $ ", getcwd(line,1024) );
            fgets( line, sizeof( line ) - 1, stdin );
            if( run( line ) == false )        // 명령어 exit,quit의 경우에만 false
                break;
        }   
        
        return 0;
}
