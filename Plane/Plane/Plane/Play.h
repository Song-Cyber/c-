#include <graphics.h>  // 替换为正确的EasyX头文件




IMAGE img[4];//图片数组

//我方飞机大小
#define myplanewidth  25
#define myplaneheight 40
//enemy size
#define enemywidth    25
#define enemyheight   25
//bullet size
#define bulletwidth   25
#define bulletheight  25

// 初始化函数
void init()
{
    //载入图片
    loadimage(&img[0], "res\\游戏背景.png", 400, 600);
    loadimage(&img[1], "res\\飞机.png", myplanewidth, myplaneheight);
    loadimage(&img[1], "res\\敌人.png", enemywidth, enemyheight);
    loadimage(&img[1], "res\\攻击.png", bulletwidth, bulletheight);
}

void start()
{
    initgraph(400,600);  // 初始化图形窗口（窗口大小）

    //初始化
    while (1);

    closegraph();
}
