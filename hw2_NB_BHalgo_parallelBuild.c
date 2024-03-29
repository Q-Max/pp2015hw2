#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <pthread.h>
//#include <unistd.h>
#include <sys/time.h>
#include <math.h>


#define G 6.67e-11
#define EPSILON 1e-7
typedef struct nodeStruct{
	double x, y, dx, dy, xc, yc;
	unsigned int count;
	struct nodeStruct *tl, *tr, *bl, *br;
	pthread_mutex_t mutex;
} node;
struct body{
	double x, y, vx, vy;
};
struct acce{
	double ax,ay;
};
struct point{
	int x, y;
};
struct slice{
	int start, end;
};
int *start, *end;
node *root;
char *list;
void initGraph(int width,int height);
inline void draw(int x,int y);
inline void clear(struct point* points, int N);
inline void computeAcce();
//void *workAcc(void* arg);
void *workAcc2(void* arg);
void *workAccBH(void* arg);
//void *workPoi(void* arg);
void *workPoi2(void* arg);
void *wrapbuild(void* arg);
//void *workDraw(void* arg);
inline void findEdges();
inline node* newNode();
void buildTree(node *p, double x, double y, double dx, double dy, double x_body, double y_body);
inline void delTree(node *p);
int cmp(const void* a, const void* b);
int cmp(const void* a, const void* b){	
	if((*((struct body*)a)).x>(*((struct body*)b)).x)return 1;
	return -1;
}
void drawTree(node *p);
struct acce computeAcce_BH(node *p, double x, double y);

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
int x11Length;
double xmin, ymin, theta;
double xmax_bodies, xmin_bodies, ymax_bodies, ymin_bodies;
inline void findEdges(){
	int i;
	xmin_bodies = ymin_bodies = 1e20;
	xmax_bodies = ymax_bodies = -1e20;
	for(i=0;i<N;i++){
		if(bodies[i].x > xmax_bodies)
			xmax_bodies = bodies[i].x;
		if(bodies[i].x < xmin_bodies)
			xmin_bodies = bodies[i].x;
		if(bodies[i].y > ymax_bodies)
			ymax_bodies = bodies[i].y;
		if(bodies[i].y < ymin_bodies)
			ymin_bodies = bodies[i].y;
	}
}
inline node* newNode(){
	node* p= (node*)malloc(sizeof(node));
	p->count = 0;
	p->tl = p->tr = p->bl = p->br = NULL;
	//p->mutex = (pthread_mutex*)malloc(sizeof(pthread_mutex));
	pthread_mutex_init (&(p->mutex), NULL);
	return p;
}
void *wrapbuild(void* arg){
	int i;
	int tid = *(int*)arg;
	for(i=start[tid];i<end[tid];i++){
		pthread_mutex_lock(&(root->mutex));
		buildTree(root, xmin_bodies, ymin_bodies, xmax_bodies - xmin_bodies, ymax_bodies - ymin_bodies, bodies[i].x, bodies[i].y);
		pthread_mutex_unlock(&(root->mutex));
	}
	return NULL;
}
void buildTree(node *p, double x, double y, double dx, double dy, double x_body, double y_body){
	//printf("%u\n",(unsigned int)p);
	//pthread_mutex_lock(&(p->mutex));
	double xc, yc;
	if(p == NULL)
		puts("QQ");
	if(p->count==0){		
		p->x = x;
		p->y = y;
		p->xc = x_body;
		p->yc = y_body;
		p->dx = dx;
		p->dy = dy;
		p->count = 1;
		p->tl = p->tr = p->bl = p->br = NULL;
		//pthread_mutex_unlock(&(p->mutex));
		return;
	}
	else if(p->count==1){
		/*if(p->xc > p->x && p->xc <= p->x + (p->d)/2){
			if(p->yc > p->y && p->yc <= p->y + (p->d)/2){
				buildTree(p->tl, p->x, p->y, (p->d)/2, p->xc, p->yc);
			}
			//else 
			else if(p->yc > (p->y)+d/2 && p->yc < p->y + p->d){
				buildTree(p->bl, p->x, p->y + (p->d)/2, (p->d)/2, p->xc, p->yc);
			}
		}
		else if(p->xc > p->x + (p->d)/2 && p->xc <= p->x + p->d){
			if(p->yc > p->y && p->yc <= p->y + (p->d)/2){
				buildTree(p->tr, p->x + (p->d)/2, p->y, (p->d)/2, p->xc, p->yc);
			}
			//else 
			else if(p->yc > (p->y)+d/2 && p->yc < p->y + p->d){
				buildTree(p->br, p->x + (p->d)/2, p->y + (p->d)/2, (p->d)/2, p->xc, p->yc);
			}
		}*/
		xc = p->xc;
		yc = p->yc;
		p->xc = p->xc * p->count / (p->count+1) + x_body / (p->count+1);
		p->yc = p->yc * p->count / (p->count+1) + y_body / (p->count+1);
		p->count++;
		if(xc < p->x + (p->dx)/2){
			if(yc < p->y + (p->dy)/2){
				p->tl = newNode();
				pthread_mutex_lock(&(p->tl->mutex));
				pthread_mutex_unlock(&(p->mutex));
				buildTree(p->tl, p->x, p->y, (p->dx)/2, (p->dy)/2, xc, yc);
				pthread_mutex_unlock(&(p->tl->mutex));
				pthread_mutex_lock(&(p->mutex));
			}
			else{
				p->bl = newNode();
				pthread_mutex_lock(&(p->bl->mutex));
				pthread_mutex_unlock(&(p->mutex));
				buildTree(p->bl, p->x, p->y + (p->dy)/2, (p->dx)/2, (p->dy)/2, xc, yc);
				pthread_mutex_unlock(&(p->bl->mutex));
				pthread_mutex_lock(&(p->mutex));
			}
		}
		else {
			if(yc < p->y + (p->dy)/2){
				p->tr = newNode();
				pthread_mutex_lock(&(p->tr->mutex));
				pthread_mutex_unlock(&(p->mutex));
				buildTree(p->tr, p->x + (p->dx)/2, p->y, (p->dx)/2, (p->dy)/2, xc, yc);
				pthread_mutex_unlock(&(p->tr->mutex));
				pthread_mutex_lock(&(p->mutex));
			}
			else{
				p->br = newNode();
				pthread_mutex_lock(&(p->br->mutex));
				pthread_mutex_unlock(&(p->mutex));
				buildTree(p->br, p->x + (p->dx)/2, p->y + (p->dy)/2, (p->dx)/2, (p->dy)/2, xc, yc);
				pthread_mutex_unlock(&(p->br->mutex));
				pthread_mutex_lock(&(p->mutex));
			}
		}
	}
	else{
		p->xc = p->xc * p->count / (p->count+1) + x_body / (p->count+1);
		p->yc = p->yc * p->count / (p->count+1) + y_body / (p->count+1);
		p->count++;
	}
	// p->xc = p->xc * p->count / (p->count+1) + x_body / (p->count+1);
	// p->yc = p->yc * p->count / (p->count+1) + y_body / (p->count+1);
	// p->count++;
	/*
	if(x_body > p->x && x_body <= p->x + (p->d)/2){
		if(y_body > p->y && y_body <= p->y + (p->d)/2){
			buildTree(p->tl, p->x, p->y, (p->d)/2, x_body, y_body);
		}
		//else 
		else if(y_body > (p->y)+d/2 && y_body < p->y + p->d){
			buildTree(p->bl, p->x, p->y + (p->d)/2, (p->d)/2, x_body, y_body);
		}
	}
	else if(x_body > p->x + (p->d)/2 && x_body <= p->x + p->d){
		if(y_body > p->y && y_body <= p->y + (p->d)/2){
			buildTree(p->tr, p->x + (p->d)/2, p->y, (p->d)/2, x_body, y_body);
		}
		//else 
		else if(y_body > (p->y)+d/2 && y_body < p->y + p->d){
			buildTree(p->br, p->x + (p->d)/2, p->y + (p->d)/2, (p->d)/2, x_body, y_body);
		}
	}*/
	if(x_body < p->x + (p->dx)/2){
		if(y_body < p->y + (p->dy)/2){
			if(p->tl==NULL)
				p->tl = newNode();
			pthread_mutex_lock(&(p->tl->mutex));
			pthread_mutex_unlock(&(p->mutex));
			buildTree(p->tl, p->x, p->y, (p->dx)/2, (p->dy)/2, x_body, y_body);
			pthread_mutex_unlock(&(p->tl->mutex));
			pthread_mutex_lock(&(p->mutex));
		}
		else{
			if(p->bl==NULL)
				p->bl = newNode();
			pthread_mutex_lock(&(p->bl->mutex));
			pthread_mutex_unlock(&(p->mutex));
			buildTree(p->bl, p->x, p->y + (p->dy)/2, (p->dx)/2, (p->dy)/2, x_body, y_body);
			pthread_mutex_unlock(&(p->bl->mutex));
			pthread_mutex_lock(&(p->mutex));
		}
	}
	else {
		if(y_body < p->y + (p->dy)/2){
			if(p->tr==NULL)
				p->tr = newNode();
			pthread_mutex_lock(&(p->tr->mutex));
			pthread_mutex_unlock(&(p->mutex));
			buildTree(p->tr, p->x + (p->dx)/2, p->y, (p->dx)/2, (p->dy)/2, x_body, y_body);
			pthread_mutex_unlock(&(p->tr->mutex));
			pthread_mutex_lock(&(p->mutex));
		}
		else{
			if(p->br==NULL)
				p->br = newNode();
			pthread_mutex_lock(&(p->br->mutex));
			pthread_mutex_unlock(&(p->mutex));
			buildTree(p->br, p->x + (p->dx)/2, p->y + (p->dy)/2, (p->dx)/2, (p->dy)/2, x_body, y_body);
			pthread_mutex_unlock(&(p->br->mutex));
			pthread_mutex_lock(&(p->mutex));
		}
	}
	return;
}
inline void delTree(node *p){
	if(p->tl)delTree(p->tl);
	if(p->tr)delTree(p->tr);
	if(p->bl)delTree(p->bl);
	if(p->br)delTree(p->br);
	free(p);
	return;
}

void drawTree(node *p){
	if(p->tl)drawTree(p->tl);
	if(p->tr)drawTree(p->tr);
	if(p->bl)drawTree(p->bl);
	if(p->br)drawTree(p->br);
	if(p->count){
		if((int)(((p->x-xmin)*unit)) > 0 && (int)(((p->y-ymin)*unit)) > 0 && (int)(p->dx*unit) > 0 && (int)(p->dy*unit) > 0)
			XDrawRectangle( display, window, gc, (int)(((p->x-xmin)*unit)), (int)(((p->y-ymin)*unit)), (int)(p->dx*unit), (int)(p->dy*unit));
	}
	return;
}
void initGraph(int width,int height)
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
	struct timeval tvalBefore, tvalAfter, tresult;
	struct timeval iobefore, ioafter, ioresult;
	struct timeval buildtreebefore, buildtreeafter, buildtreeresult;
	buildtreeresult.tv_sec = buildtreeresult.tv_usec = 0;
	gettimeofday (&tvalBefore, NULL);
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
	theta = atof(argv[6]);
	unit = 0;
	int enableX11;
	double length = 0;
	x11Length = 0;
	unit = xmin = ymin = 0;
	int acc_t;
	FILE *fp;	
	pthread_t threads[n];


	int reminder;
	int *tids;
	node *oldroot = NULL;
	Colormap screen_colormap;
	
	XColor gray;
	struct acce at;
	if(!strcmp(argv[7],"enable"))
		enableX11 = 1;
	else if(!strcmp(argv[7],"disable"))
		enableX11 = 0;
	else{
		puts("error in X11 flag.\nEXIT");
		exit(0);
	}
	if(enableX11){
		xmin = atof(argv[8]);
		ymin = atof(argv[9]);
		length = atof(argv[10]);
		x11Length = atoi(argv[11]);
		unit = x11Length/length;
		initGraph(x11Length, x11Length);
		screen_colormap = DefaultColormap(display, DefaultScreen(display));
		XAllocNamedColor(display, screen_colormap, "gray", &gray, &gray);
	}
	constGM = G * m;
	int i, x, y;
	gettimeofday (&iobefore, NULL);
	fp = fopen(filename, "r");
	if(fp==NULL){
		puts("file error, EXIT");
		exit(0);
	}
	if(!fscanf(fp,"%d",&N)){
		puts("error in file");
		exit(0);
	}
	
	printf("%d testcases in  testfile\n", N);
	if(enableX11){
		points = (struct point*)malloc(sizeof(struct point)*N);
		for (i=0; i<N; i++){
			points[i].x = 1;
			points[i].y = 1;
		}
	}
	bodies = (struct body*)malloc(sizeof(struct body)*N);
	if(bodies==NULL){
		exit(0);
	}
	for (i=0; i<N; i++){
		if(!fscanf(fp,"%lf %lf %lf %lf",&bodies[i].x, &bodies[i].y, &bodies[i].vx, &bodies[i].vy)){
			puts("error in file");
			exit(0);
		}
	}
	gettimeofday (&ioafter, NULL);
	ioresult.tv_sec = ioafter.tv_sec-iobefore.tv_sec;
	ioresult.tv_usec = ioafter.tv_usec-iobefore.tv_usec;
	while(ioresult.tv_usec<0){
		ioresult.tv_sec--;
		ioresult.tv_usec+=1000000;
	}
	qsort(bodies, N, (sizeof(struct body)), cmp);
	start = (int*)malloc(sizeof(int)*n);
	end = (int*)malloc(sizeof(int)*n);
	tids = (int*)malloc(sizeof(int)*n);
	reminder = N % n;
	for(i=0;i<n;i++){
		if(i==0){
			if(reminder){
				start[0] = 0;
				end[0] = (N / n) + 1;
				reminder--;
			}
			else{
				start[0] = 0;
				end[0] = (N / n);
			}
		}
		else if(reminder){
			start[i] = end[i-1];
			end[i] = start[i] + N / n + 1;
			reminder--;
		}
		else{
			start[i] = end[i-1];
			end[i] = start[i] + N / n;
		}
		tids[i] = i;
	}
	if(N>10*n){
		if(enableX11){
			for (acc_t=0; acc_t<T; acc_t++) {
				gettimeofday (&buildtreebefore, NULL);
				if(oldroot!=NULL){
#ifdef grid
					XSetForeground(display,gc,BlackPixel(display,screen));
					XFillRectangle(display,window,gc,1,1,x11Length,x11Length);
#endif
					delTree(oldroot);
				}
				root = newNode();
				findEdges();
				for(i=0;i<n;i++){
					pthread_create(&threads[i], NULL, wrapbuild, (void *) &tids[i]);
				}
				for(i=0;i<n;i++){
					pthread_join(threads[i], NULL);
				}
				/*for(i=0;i<N;i++)
					buildTree(root, xmin_bodies, ymin_bodies, xmax_bodies - xmin_bodies, ymax_bodies - ymin_bodies, bodies[i].x, bodies[i].y);*/
				gettimeofday (&buildtreeafter, NULL);
				buildtreeresult.tv_sec += buildtreeafter.tv_sec - buildtreebefore.tv_sec;
				buildtreeresult.tv_usec += buildtreeafter.tv_usec - buildtreebefore.tv_usec;
				for(i=0;i<n;i++){
					pthread_create(&threads[i], NULL, workAccBH, (void *) &tids[i]);
				}
				for(i=0;i<n;i++){
					pthread_join(threads[i], NULL);
				}
				for(i=0;i<n;i++){
					pthread_create(&threads[i], NULL, workPoi2, (void *) &tids[i]);
				}
				for(i=0;i<n;i++){
					pthread_join(threads[i], NULL);
				}
#ifndef grid
				clear(points, N);
#endif
				for(i=0;i<N;i++){
					x=(bodies[i].x-xmin)*unit;
					y=(bodies[i].y-ymin)*unit;
					if(x>0&&x<x11Length&&y>0&&y<x11Length){
						draw(x,y);
						points[i].x = x;
						points[i].y = y;
					}
				}
#ifdef grid
				XSetForeground(display,gc,gray.pixel);
				drawTree(root);
#endif			
				XFlush(display);
				oldroot = root;
			}
		}
		else{
			for (acc_t=0; acc_t<T; acc_t++) {
				gettimeofday (&buildtreebefore, NULL);
				if(oldroot!=NULL){

					delTree(oldroot);
				}
				root = newNode();
				findEdges();
				for(i=0;i<N;i++)
					buildTree(root, xmin_bodies, ymin_bodies, xmax_bodies - xmin_bodies, ymax_bodies - ymin_bodies, bodies[i].x, bodies[i].y);
				gettimeofday (&buildtreeafter, NULL);
				buildtreeresult.tv_sec += buildtreeafter.tv_sec - buildtreebefore.tv_sec;
				buildtreeresult.tv_usec += buildtreeafter.tv_usec - buildtreebefore.tv_usec;
				for(i=0;i<n;i++){
					pthread_create(&threads[i], NULL, workAccBH, (void *) &tids[i]);
				}
				for(i=0;i<n;i++){
					pthread_join(threads[i], NULL);
				}
				for(i=0;i<n;i++){
					pthread_create(&threads[i], NULL, workPoi2, (void *) &tids[i]);
				}
				for(i=0;i<n;i++){
					pthread_join(threads[i], NULL);
				}
				oldroot = root;
			}
		}
	}
	else{
		puts("Using single thread because few testcase #");
		if(enableX11){
			for (acc_t=0; acc_t<T; acc_t++) {
				gettimeofday (&buildtreebefore, NULL);
				if(oldroot!=NULL){
#ifdef grid
					XSetForeground(display,gc,BlackPixel(display,screen));
					XFillRectangle(display,window,gc,1,1,x11Length,x11Length);
#endif
					delTree(oldroot);
				}
				root = newNode();
				findEdges();
				for(i=0;i<N;i++)
					buildTree(root, xmin_bodies, ymin_bodies, xmax_bodies - xmin_bodies, ymax_bodies - ymin_bodies, bodies[i].x, bodies[i].y);
				gettimeofday (&buildtreeafter, NULL);
				buildtreeresult.tv_sec += buildtreeafter.tv_sec - buildtreebefore.tv_sec;
				buildtreeresult.tv_usec += buildtreeafter.tv_usec - buildtreebefore.tv_usec;
				for(i=0;i<N;i++){
					at = computeAcce_BH(root, bodies[i].x, bodies[i].y);
					bodies[i].vx += at.ax * t;
					bodies[i].vy += at.ay * t;
				}
				for(i=0;i<N;i++){
					bodies[i].x += bodies[i].vx * t;
					bodies[i].y += bodies[i].vy * t;
				}
#ifndef grid
				clear(points, N);
#endif
				for(i=0;i<N;i++){
					x=(bodies[i].x-xmin)*unit;
					y=(bodies[i].y-ymin)*unit;
					if(x>0&&x<x11Length&&y>0&&y<x11Length){
						draw(x,y);
						points[i].x = x;
						points[i].y = y;
					}
				}
#ifdef grid
				XSetForeground(display,gc,gray.pixel);
				drawTree(root);
#endif			
				XFlush(display);
				oldroot = root;
			}
		}
		else{
			for (acc_t=0; acc_t<T; acc_t++) {
				gettimeofday (&buildtreebefore, NULL);
				if(oldroot!=NULL){
					delTree(oldroot);
				}
				root = newNode();
				findEdges();
				for(i=0;i<N;i++)
					buildTree(root, xmin_bodies, ymin_bodies, xmax_bodies - xmin_bodies, ymax_bodies - ymin_bodies, bodies[i].x, bodies[i].y);
				gettimeofday (&buildtreeafter, NULL);
				buildtreeresult.tv_sec += buildtreeafter.tv_sec - buildtreebefore.tv_sec;
				buildtreeresult.tv_usec += buildtreeafter.tv_usec - buildtreebefore.tv_usec;
				for(i=0;i<N;i++){
					at = computeAcce_BH(root, bodies[i].x, bodies[i].y);
					bodies[i].vx += at.ax * t;
					bodies[i].vy += at.ay * t;
				}
				oldroot = root;
			}
		}
	}
	
	while(buildtreeresult.tv_usec<0){
		buildtreeresult.tv_sec--;
		buildtreeresult.tv_usec+=1000000;
	}
	while(buildtreeresult.tv_usec>1000000){
		buildtreeresult.tv_sec++;
		buildtreeresult.tv_usec-=1000000;
	}
	gettimeofday (&tvalAfter, NULL);
	tresult.tv_sec = tvalAfter.tv_sec-tvalBefore.tv_sec;
	tresult.tv_usec = tvalAfter.tv_usec-tvalBefore.tv_usec;
	if(tresult.tv_usec<0){
		tresult.tv_sec--;
		tresult.tv_usec+=1000000;
	}
	printf("IO cost %ld sec %ld millisec.\n", (ioresult.tv_sec), (ioresult.tv_usec)/1000);
	printf("Building tree cost %ld sec %ld millisec.\n", (buildtreeresult.tv_sec), (buildtreeresult.tv_usec)/1000);
	printf("Total cost %ld sec %ld millisec.\n\n", (tresult.tv_sec), (tresult.tv_usec)/1000);
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
void *workAcc2(void* arg){
	int tid = *(int*)arg;
	int i, j;
	double axt, ayt, r;
	for(i=start[tid];i<end[tid];i++){
		axt=0;
		ayt=0;
		for(j=0;j<N;j++){
			if(i==j)
				continue;
			r = sqrt((bodies[i].x-bodies[j].x)*(bodies[i].x-bodies[j].x) + (bodies[i].y-bodies[j].y)*(bodies[i].y-bodies[j].y));
			axt += constGM * (bodies[j].x-bodies[i].x) / (r*r*r+EPSILON);
			ayt += constGM * (bodies[j].y-bodies[i].y) / (r*r*r+EPSILON);
		}
		bodies[i].vx += axt * t;
		bodies[i].vy += ayt * t;
	}
	pthread_exit(NULL);
	return NULL;
}
void *workAccBH(void* arg){
	int tid = *(int*)arg;
	int i;
	//double axt, ayt, r;
	struct acce at;
	for(i=start[tid];i<end[tid];i++){
		at = computeAcce_BH(root, bodies[i].x, bodies[i].y);
		bodies[i].vx += at.ax * t;
		bodies[i].vy += at.ay * t;
	}
	pthread_exit(NULL);
	return NULL;
}
void *workPoi2(void* arg){
	int tid = *(int*)arg;
	int i;
	for(i=start[tid];i<end[tid];i++){
		bodies[i].x += bodies[i].vx * t; // compute new position
		bodies[i].y += bodies[i].vy * t; // compute new position
	}
	pthread_exit(NULL);
	return NULL;
}
struct acce computeAcce_BH(node *p, double x, double y){
	struct acce at, a1;
	double d = p->dx>p->dy?p->dx:p->dy;
	at.ax = at.ay = 0;
	if(p->xc==x&&p->yc==y){
		return at;
	}
	double r = sqrt((x-p->xc)*(x-p->xc) + (y-p->yc)*(y-p->yc));
	if(d/r<=theta||p->count==1){
		at.ax = p->count * constGM * (p->xc - x) / (r*r*r+EPSILON);
		at.ay = p->count * constGM * (p->yc - y) / (r*r*r+EPSILON);
		return at;
	}
	else{
		if(p->tl){
			a1 = computeAcce_BH(p->tl, x, y);
			at.ax += a1.ax;
			at.ay += a1.ay;
		}
		if(p->bl){
			a1 = computeAcce_BH(p->bl, x, y);
			at.ax += a1.ax;
			at.ay += a1.ay;
		}
		if(p->tr){
			a1 = computeAcce_BH(p->tr, x, y);
			at.ax += a1.ax;
			at.ay += a1.ay;
		}
		if(p->br){
			a1 = computeAcce_BH(p->br, x, y);
			at.ax += a1.ax;
			at.ay += a1.ay;
		}
		return at;
	}
}