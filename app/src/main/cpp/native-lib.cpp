#include <jni.h>
#include <string>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include <pthread.h>

float width_scale=1;
float height_scale=1;

#define ZOMBIE_WIDTH 115
#define ZOMBIE_HEIGHT 170

#define SCALE_WIDTH 1.44
#define SCALE_HEIGHT 123

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "jni_draw", __VA_ARGS__))


struct zombie_data
{
    int x;  //僵尸x坐标
    int y;  //僵尸y坐标,这里y为0,1,2,3,4,5....
};

int udp_socket_fd;
//这个z_data就是接收udp数据的变量,默认一次接收25个数据
struct zombie_data z_data[25];

void setup_udp(){
    int port = 53316;
    udp_socket_fd = socket(AF_INET,SOCK_DGRAM,0);
    //设置udp非阻塞
//    int flag=fcntl(udp_socket_fd, F_GETFL, 0);
//    fcntl(udp_socket_fd, F_SETFL, flag | O_NONBLOCK);
    if(udp_socket_fd < 0 )
    {
        perror("creat socket fail\n");
        return;
    }
    struct sockaddr_in local_addr = {0};
    local_addr.sin_family  = AF_INET; //使用IPv4协议
    local_addr.sin_port	= port;   //网络通信都使用大端格式
    local_addr.sin_addr.s_addr = INADDR_ANY;//让系统检测本地网卡，自动绑定本地IP
    if(bind(udp_socket_fd,(struct sockaddr*)&local_addr,sizeof(local_addr)) < 0)
    {
        perror("bind fail:");
        close(udp_socket_fd);
        return;
    }
}



extern "C"
JNIEXPORT void JNICALL
Java_com_shocker_jnidraw_OverlayView_Draw(JNIEnv *env, jclass clazz, jobject overlay_view,
                                          jobject canvas) {
    // TODO: implement Draw()
    jclass canvasView = env->GetObjectClass(overlay_view);
    jmethodID drawrect = env->GetMethodID(canvasView, "DrawRect",
                                           "(Landroid/graphics/Canvas;IIIIFFFFF)V");
    for(int i=0;i< sizeof(z_data)/ sizeof(struct zombie_data);i++){
        if (z_data[i].x<=0){
            continue;
        }
        float zombie_x=z_data[i].x*SCALE_WIDTH*width_scale;
        float zombie_y=z_data[i].y*SCALE_HEIGHT*height_scale;
        env->CallVoidMethod(overlay_view, drawrect, canvas,255, 255,69, 0,
                            (float)2.5,(float)zombie_x, (float)zombie_y,
                            (float)zombie_x+ZOMBIE_WIDTH*width_scale, (float)zombie_y+ZOMBIE_HEIGHT*height_scale);
    }
//    env->CallVoidMethod(overlay_view, drawrect, canvas,255, 255,255, 255,
//                        (float)2,(float)80, (float)0, (float)195, (float)170);
}

//阻塞接收root进程发来的数据
void *recv_udp(void *)
{
    while (true)
    {
        //理论上来说,这里是多线程访问同一数据,需要加锁,这里简化处理.
        recvfrom(udp_socket_fd, z_data, sizeof(z_data), 0, NULL, NULL);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_shocker_jnidraw_OverlayView_NativeInit(JNIEnv *env, jobject thiz) {
    // TODO: implement NativeInit()
    //初始化udp
    setup_udp();
    //创建一个线程专门来读取udp数据
    pthread_t tidp;
    if (pthread_create(&tidp, NULL, recv_udp, NULL) == -1)
    {
        printf("pthread_create error");
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_shocker_jnidraw_OverlayView_SetWidthScale(JNIEnv *env, jobject thiz, jint width) {
    // TODO: implement SetWidthScale()
    //设置缩放倍率,测试在1280*720做的,不同分辨率要做不同处理
    width_scale=(float)width/1280;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_shocker_jnidraw_OverlayView_SetHeightScale(JNIEnv *env, jobject thiz, jint height) {
    // TODO: implement SetHeightScale()
    //同上
    height_scale=(float)height/720;
//    height_scale=width_scale;
}