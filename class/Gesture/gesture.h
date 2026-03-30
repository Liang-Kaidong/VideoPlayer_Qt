#ifndef GESTURE_H
#define GESTURE_H

#include <QObject>
#include <QStackedWidget>
#include <QSet>
#include <QWidget>
#include <QList>

class QLabel;

class Gesture : public QObject
{
    Q_OBJECT

public:


    explicit Gesture(QStackedWidget *stackedWidget, QObject *parent = nullptr);

    void openApp(int pageIndex);                       // 打开页面（缩放动画）
    void closeApp(int pageIndex);                      // 关闭页面（缩放动画）
    void slidePageLeftToRight(int pageIndex);          // 从左往右滑页
    void slidePageRightToLeft(int pageIndex);          // 从右往左滑页
    void slidePageUpToDown(int pageIndex);             // 从上到下滑页
    void slidePageDownToUp(int pageIndex);             // 从下到上滑页

    void addVerticalScrollWidget(QWidget *widget);     // 注册纵向可滚动控件
    void addHorizontalScrollWidget(const QList<int> &pageList); // 注册可横向滑动的页面

protected:
    bool eventFilter(QObject *targetObject, QEvent *event) override;

private:

    enum SlideDirection {
        SlideLeftToRight,   // 从左向右滑动（X轴）
        SlideRightToLeft,   // 从右向左滑动（X轴）
        SlideUpToDown,      // 从上向下滑动（Y轴）
        SlideDownToUp       // 从下向上滑动（Y轴）
    };

    // 只保留带方向参数的动画函数
    bool startSwitchAnimation(int fromPageIndex, int toPageIndex, SlideDirection direction);

    // 计算小矩形（用于缩放动画）
    QRect calculateSmallRect(const QRect &originalRect);

    QStackedWidget *targetStackedWidget = nullptr;  // 要控制的堆叠窗口
    QLabel *animationOverlayLabel = nullptr;        // 缩放动画的覆盖标签
    QWidget *animationBlockerWidget = nullptr;      // 动画时屏蔽操作的透明控件

    int dragStartX = 0;          // 拖动起始点X坐标
    int dragStartY = 0;          // 拖动起始点Y坐标
    int lastDragDeltaX = 0;      // 最后一次拖动的X轴偏移量
    int swipeThresholdValue = 50;// 滑动触发切页的阈值（像素）
    bool isAnimationRunning = false; // 是否正在播放动画
    bool isDragging = false;         // 是否正在拖动页面

    QSet<QWidget*> verticalScrollWidgetSet;  // 可纵向滚动的控件集合
    QList<int> allowHorizontalSwitchPageList; // 允许横向滑动的页面索引列表

    QWidget *animationFromWidget = nullptr;  // 动画离开的页面
    QWidget *animationToWidget = nullptr;    // 动画进入的页面

    int stackedWidgetWidth = 0;   // 堆叠窗口宽度（缓存）
    int stackedWidgetHeight = 0;  // 堆叠窗口高度（缓存）

private slots:
    void onSwitchAnimationFinished();
    void onOpenAppAnimationFinished();
    void onCloseAppAnimationFinished();

};

#endif
