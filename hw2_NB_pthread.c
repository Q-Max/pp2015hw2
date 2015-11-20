#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <pthread.h>
//#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <float.h>

#define G 6.67e-11

struct body{
	double x, y, vx, vy;
};
struct point{
	int x, y;
};
struct slice{
	int start, end;
};
inline void initGraph(int width,int height);
inline void draw(int x,int y);
inline void clear(struct point* points, int N);
void *workAcc(void* arg);
void *workPoi(void* arg);
void *workDraw(void* arg);

GC gc;
Display *display;
Window window;      //initialization for a window
int screen;         //which screen 
double constGM;
double t;
struct body *bodies;
struct point *points;
int n, N;
double unit;
int x11Length, xmin, ymin;
inline void initGraph(int width,int height)
{
	/* open connection with the server */ 
	display = XOpenDisplay(NULL);
	if(display == NULL) {
		fprintf(stderr, "cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);

	/* set window position */
	int x = 0;
	int y = 0;

	/* border width in pixels */
	int border_width = 0;

	/* create window */
	window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width, BlackPixel(display, screen), WhitePixel(display, screen));
	
	/* create graph */
	XGCValues values;
	long valuemask = 0;
	
	gc = XCreateGC(display, window, valuemask, &values);
	//XSetBackground (display, gc, WhitePixel (display, screen));
	XSetForeground (display, gc, BlackPixel (display, screen));
	XSetBackground(display, gc, 0X0000FF00);
	XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
	
	/* map(show) the window */
	XMapWindow(display, window);
	XSync(display, 0);

	/* draw rectangle */
	XSetForeground(display,gc,BlackPixel(display,screen));
	XFillRectangle(display,window,gc,0,0,width,height);
	XFlush(display);
}

inline void draw(int x,int y)
{
	/* draw point */
	XSetForeground(display,gc,WhitePixel(display,screen));
	XDrawPoint (display, window, gc, x, y);
}

int main(int argc,char *argv[])
{
	if(argc<8||(argc>8&&argc!=12)){
		puts("error in arguments");
		puts("\"./hw2_xxx #threads m T t FILE theta disable\" or");
		puts("\"./hw2_xxx #threads m T t FILE theta enable xmin ymin length Length\"");
		exit(0);

	}
	n = atoi(argv[1]);
	const double m = atof(argv[2]);
	const int T = atoi(argv[3]);
	t = atof(argv[4]);
	const char *filename = argv[5];
	const double theta = atof(argv[6]);
	unit = 0;
	int enableX11;
	int length;
	x11Length = 0;
	int acc_t;	
	FILE *fp;
	struct body *bodies_main;
	pthread_t threads[n];
	int load;
	if(!strcmp(argv[7],"enable"))
		enableX11 = 1;
	else if(!strcmp(argv[7],"disable"))
		enableX11 = 0;
	else{
		puts("error in X11 flag.\nEXIT");
		exit(0);
	}
	if(enableX11){
		xmin = atoi(argv[8]);
		ymin = atoi(argv[9]);
		length = atoi(argv[10]);
		x11Length = atoi(argv[11]);
		unit = x11Length/length;
		initGraph(x11Length, x11Length);		
	}
	constGM = G * m;
	int i, x, y;
	fp = fopen(filename, "r");
	if(fp==NULL){
		puts("file error, EXIT");
		exit(0);
	}
	if(!fscanf(fp,"%d",&N)){
		puts("error in file");
		exit(0);
	}
	load = N/n;
	if(enableX11){
		points = (struct point*)malloc(sizeof(struct point)*N);
		for (i=0; i<N; i++){
			points[i].x = 1;
			points[i].y = 1;
		}
	}
	bodies_main = (struct body*)malloc(sizeof(struct body)*N);
	if(bodies_main==NULL){
		exit(0);
	}
	for (i=0; i<N; i++){
		if(!fscanf(fp,"%lf %lf %lf %lf",&bodies_main[i].x, &bodies_main[i].y, &bodies_main[i].vx, &bodies_main[i].vy)){
			puts("error in file");
			exit(0);
		}
	}
	struct timeval tvalBefore, tvalAfter, tresult;
	gettimeofday (&tvalBefore, NULL);
	bodies = bodies_main;
	struct slice *slices = (struct slice*)malloc(sizeof(struct slice)*n);
	for(i=0;i<n;i++){
		slices[i].start = load*i;
		slices[i].end = load*(i+1);
		//if(reminder!=0)slices[i].end+=1;
	}
	slices[i-1].end = load*(i)+N%n;
	if(enableX11){
		for (acc_t=0; acc_t<T; acc_t++) {

			for(i=0;i<n;i++){
				pthread_create(&threads[i], NULL, workAcc, (void *) &slices[i]);
			}
			clear(points, N);
			for(i=0;i<n;i++){
				pthread_join(threads[i], NULL);
			}
			for(i=0;i<n;i++){
				pthread_create(&threads[i], NULL, workPoi, (void *) &slices[i]);
			}
			for(i=0;i<n;i++){
				pthread_join(threads[i], NULL);
			}
			/*for (i=0; i<N; i++) {
				bodies[i].x += bodies[i].vx * t; // compute new position
				bodies[i].y += bodies[i].vy * t; // compute new position
			}*/
			
			/*for(i=0;i<n;i++){
				pthread_create(&threads[i], NULL, workDraw, (void *) &slices[i]);
			}
			for(i=0;i<n;i++){
				pthread_join(threads[i], NULL);
			}*/
			for(i=0;i<N;i++){
				x=(bodies[i].x-xmin)*unit;
				y=(bodies[i].y-ymin)*unit;
				if(x>0&&x<x11Length&&y>0&&y<x11Length){
					draw(x,y);
					points[i].x = x;
					points[i].y = y;
				}
			}
			XFlush(display);
		}
	}
	else{
		for (acc_t=0; acc_t<T; acc_t++) {
			for(i=0;i<n;i++){
				pthread_create(&threads[i], NULL, workAcc, (void *) &slices[i]);
			}
			for(i=0;i<n;i++){
				pthread_join(threads[i], NULL);
			}
			for(i=0;i<n;i++){
				pthread_create(&threads[i], NULL, workPoi, (void *) &slices[i]);
			}
			for(i=0;i<n;i++){
				pthread_join(threads[i], NULL);
			}
			/*for (i=0; i<N; i++) {
				bodies[i].x += bodies[i].vx * t; // compute new position
				bodies[i].y += bodies[i].vy * t; // compute new position
			}*/
		}
	}	
	gettimeofday (&tvalAfter, NULL);
	tresult.tv_sec = tvalAfter.tv_sec-tvalBefore.tv_sec;
    tresult.tv_usec = tvalAfter.tv_usec-tvalBefore.tv_usec;
    if(tresult.tv_usec<0){
        tresult.tv_sec--;
        tresult.tv_usec+=1000000;
    }
    printf("Finish at %ld sec %ld millisec.\n", (tresult.tv_sec), (tresult.tv_usec)/1000);
	return 0;
}

inline void clear(struct point *points, int N)
{
	/* draw point */
	int i;
	for(i=0;i<N;i++){
		XSetForeground(display,gc,BlackPixel(display,screen));
		XDrawPoint (display, window, gc, points[i].x, points[i].y);
	}

}
void *workAcc(void* arg){
	struct slice *slices = (struct slice*)arg;
	int i, j;
	double axt, ayt, r, a;
	for(i=slices->start;i<slices->end;i++){
		axt=0;
		ayt=0;
		for(j=0;j<N;j++){
			if(i==j)
				continue;
			/*r =  sqrt((bodies[i].x-bodies[j].x)*(bodies[i].x-bodies[j].x) + (bodies[i].y-bodies[j].y)*(bodies[i].y-bodies[j].y));
			if(r==0)
				puts("OAQQQQQQQQQQQQQQQQQQQQQQQQQ");
			else{*/
				/*t2 = (((constGM * (bodies[j].x-bodies[i].x) / r) / r) / r);
				if(t2==-INFINITY)
					t2 = LDBL_MIN;
				else if(t2==INFINITY)
					t2 = LDBL_MAX;
				axt += t2;
				t2 = (((constGM * (bodies[j].y-bodies[i].y) / r) / r) / r);
				if(t2==-INFINITY)
					t2 = LDBL_MIN;
				else if(t2==INFINITY)
					t2 = LDBL_MAX;
				ayt += t2;*/
			/*	axt += (((constGM * (bodies[j].x-bodies[i].x) / r) / r) / r);
				ayt += (((constGM * (bodies[j].y-bodies[i].y) / r) / r) / r);
			}*/
			r=(bodies[i].x-bodies[j].x)*(bodies[i].x-bodies[j].x)+(bodies[i].y-bodies[j].y)*(bodies[i].y-bodies[j].y)+1e-5;
			a=constGM/r;
			axt+=a*(bodies[j].x-bodies[i].x)/sqrt(r);
			ayt+=a*(bodies[j].y-bodies[i].y)/sqrt(r);
		}
		bodies[i].vx += axt * t;
		bodies[i].vy += ayt * t;
	}
	pthread_exit(NULL);
	return NULL;
}
void *workPoi(void* arg){
	struct slice *slices = (struct slice*)arg;
	int i;
	for(i=slices->start;i<slices->end;i++){
		bodies[i].x += bodies[i].vx * t; // compute new position
		bodies[i].y += bodies[i].vy * t; // compute new position
	}
	pthread_exit(NULL);
	return NULL;	
}
void *workDraw(void* arg){
	struct slice *slices = (struct slice*)arg;
	int i, x, y;
	for(i=slices->start;i<slices->end;i++){
		x=(bodies[i].x-xmin)*unit;
		y=(bodies[i].y-ymin)*unit;
		if(x>0&&x<x11Length&&y>0&&y<x11Length){
			draw(x,y);
			points[i].x = x;
			points[i].y = y;
		}
	}
	pthread_exit(NULL);
	return NULL;
}