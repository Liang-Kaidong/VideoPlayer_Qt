#include "gesture.h"
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QScrollArea>
#include <QScroller>
#include <QTextBrowser>
#include <QApplication>
#include <QLabel>
#include <QScrollBar>
#include <algorithm>
#include <QPainter>
#include <QListWidget>
#include <QAbstractItemView>

Gesture::Gesture(QStackedWidget *stackedWidget, QObject *parent)
    : QObject(parent), targetStackedWidget(stackedWidget)
{
    if (targetStackedWidget == nullptr) {
        return;
    }

    targetStackedWidget->installEventFilter(this);

    stackedWidgetWidth = targetStackedWidget->width();
    stackedWidgetHeight = targetStackedWidget->height();

    animationBlockerWidget = new QWidget(targetStackedWidget);
    animationBlockerWidget->setGeometry(targetStackedWidget->rect());

    animationBlockerWidget->setAttribute(Qt::WA_TransparentForMouseEvents); // 鼠标穿透
    animationBlockerWidget->setAttribute(Qt::WA_NoSystemBackground);        // 无系统背景
    animationBlockerWidget->hide(); // 初始隐藏
    animationBlockerWidget->raise(); // 提升到最上层
}

void Gesture::addVerticalScrollWidget(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }

    // 如果是 QListWidget 控件，确保启用滚动
    QListWidget *listWidget = qobject_cast<QListWidget*>(widget);
    if (listWidget) {
        // 使其支持垂直滚动
        listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);  // 更平滑的滚动

        // 注册垂直滚动手势
        QScroller::grabGesture(listWidget, QScroller::TouchGesture);
        QScroller::grabGesture(listWidget, QScroller::LeftMouseButtonGesture);
    }

    // 如果是 QTextBrowser 控件，确保启用滚动
    QTextBrowser *textBrowser = qobject_cast<QTextBrowser*>(widget);
    if (textBrowser) {
        // 注册垂直滚动手势
        QScroller::grabGesture(textBrowser, QScroller::TouchGesture);
        QScroller::grabGesture(textBrowser, QScroller::LeftMouseButtonGesture);
    }

    // 如果是 QScrollArea 控件，确保启用滚动
    QScrollArea *scrollArea = qobject_cast<QScrollArea*>(widget);
    if (scrollArea) {
        QWidget *viewWidget = scrollArea->widget();  // 获取显示内容的控件
        if (viewWidget != nullptr) {
            // 获取 QScrollArea 的视口
            QWidget *viewportWidget = scrollArea->viewport();
            if (viewportWidget != nullptr) {
                // 注册滚动手势到视口
                QScroller::grabGesture(viewportWidget, QScroller::TouchGesture);
                QScroller::grabGesture(viewportWidget, QScroller::LeftMouseButtonGesture);
            }
        }
    }
}

// 注册可横向滑动的页面
void Gesture::addHorizontalScrollWidget(const QList<int> &pageList)
{
    // 保存页面列表并排序（方便后续查找）
    allowHorizontalSwitchPageList = pageList;
    std::sort(allowHorizontalSwitchPageList.begin(), allowHorizontalSwitchPageList.end());
}

// 计算小矩形
QRect Gesture::calculateSmallRect(const QRect &originalRect)
{
    int smallWidth = originalRect.width() / 10;
    int smallHeight = originalRect.height() / 10;
    int smallX = originalRect.center().x() - smallWidth;
    int smallY = originalRect.center().y() - smallHeight;

    // 返回小矩形（宽高是原矩形的1/5）
    return QRect(smallX, smallY, smallWidth * 2, smallHeight * 2);
}

// 事件过滤器
bool Gesture::eventFilter(QObject *targetObject, QEvent *event)
{
    // 处理堆叠窗口的尺寸变化（更新缓存）
    if (targetObject == targetStackedWidget && event->type() == QEvent::Resize) {
        stackedWidgetWidth = targetStackedWidget->width();
        stackedWidgetHeight = targetStackedWidget->height();
        if (animationBlockerWidget != nullptr) {
            animationBlockerWidget->setGeometry(targetStackedWidget->rect());
        }
    }

    // 正在播放动画时，不处理任何事件
    if (isAnimationRunning == true) {
        return false;
    }

    // 将目标对象转为控件
    QWidget *watchedWidget = qobject_cast<QWidget*>(targetObject);
    if (watchedWidget == nullptr) {
        return false;
    }

    // 判断是否为可滚动控件（避免滑动冲突）
    bool isInScrollWidget = false;
    for (QWidget *widget : verticalScrollWidgetSet) {
        if (widget == watchedWidget || widget->isAncestorOf(watchedWidget)) {
            isInScrollWidget = true;
            break;
        }
    }

    // 获取当前页面索引
    int currentPageIndex = targetStackedWidget->currentIndex();
    if (!allowHorizontalSwitchPageList.contains(currentPageIndex)) {
        return false;
    }

    int widgetWidth = stackedWidgetWidth;
    QWidget *currentPageWidget = targetStackedWidget->widget(currentPageIndex);
    int targetPageIndex = -1;

    // 找到当前页面在允许列表中的位置
    int currentPagePos = allowHorizontalSwitchPageList.indexOf(currentPageIndex);
    if (currentPagePos != -1 && allowHorizontalSwitchPageList.size() > 1) {
        if (currentPagePos == 0) {
            targetPageIndex = allowHorizontalSwitchPageList.at(1);
        } else if (currentPagePos == allowHorizontalSwitchPageList.size() - 1) {
            targetPageIndex = allowHorizontalSwitchPageList.at(currentPagePos - 1);
        } else {
            targetPageIndex = (lastDragDeltaX < 0) ? allowHorizontalSwitchPageList.at(currentPagePos + 1) : allowHorizontalSwitchPageList.at(currentPagePos - 1);
        }
    }

    if (targetPageIndex < 0 || targetPageIndex >= targetStackedWidget->count()) {
        return false;
    }
    QWidget *targetPageWidget = targetStackedWidget->widget(targetPageIndex);

    // 处理鼠标按下事件（开始拖动）
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        dragStartX = mouseEvent->globalX();
        dragStartY = mouseEvent->globalY();
        lastDragDeltaX = 0;
        isDragging = true; // 标记为正在拖动

        // 设置目标页面的位置（滑入前的初始位置）
        targetPageWidget->setGeometry(targetStackedWidget->rect());
        targetPageWidget->move(currentPageIndex < targetPageIndex ? widgetWidth : -widgetWidth, 0);
        targetPageWidget->show(); // 显示目标页面
        targetPageWidget->raise(); // 提升到最上层
    }
    // 处理鼠标移动事件（拖动过程）
    else if (event->type() == QEvent::MouseMove && isDragging == true) {
        if (isInScrollWidget == true) {
            return false;
        }

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        int dragDeltaX = mouseEvent->globalX() - dragStartX;
        lastDragDeltaX = dragDeltaX;

        // 限制偏移量范围
        int clampedDeltaX = dragDeltaX;
        if (clampedDeltaX < -widgetWidth) {
            clampedDeltaX = -widgetWidth;
        } else if (clampedDeltaX > widgetWidth) {
            clampedDeltaX = widgetWidth;
        }

        // 移动当前页面和目标页面
        currentPageWidget->move(clampedDeltaX, 0);
        targetPageWidget->move((currentPageIndex < targetPageIndex ? widgetWidth : -widgetWidth) + clampedDeltaX, 0);

        // 确保目标页面在拖动时始终可见
        targetPageWidget->show();
        targetPageWidget->raise();

        return true; // 拦截事件，不再传递
    }
    // 处理鼠标释放事件（结束拖动）
    else if (event->type() == QEvent::MouseButtonRelease && isDragging == true) {
        if (abs(lastDragDeltaX) < 8) {
            isDragging = false;
            return false;
        }

        // 判断是否触发切页动画
        if ((lastDragDeltaX < -swipeThresholdValue && currentPageIndex < targetPageIndex) ||
            (lastDragDeltaX > swipeThresholdValue && currentPageIndex > targetPageIndex)) {
            SlideDirection direction = (currentPageIndex < targetPageIndex) ? SlideRightToLeft : SlideLeftToRight;
            startSwitchAnimation(currentPageIndex, targetPageIndex, direction);
        } else {
            currentPageWidget->move(0, 0);
            targetPageWidget->hide(); // 不隐藏目标页面控件
        }

        isDragging = false;
        return true;
    }

    return false; // 不拦截其他事件
}

// 带方向的切页动画
bool Gesture::startSwitchAnimation(int fromPageIndex, int toPageIndex, SlideDirection direction)
{
    if (isAnimationRunning == true) {
        return false;
    }

    QWidget *fromPageWidget = targetStackedWidget->widget(fromPageIndex);
    QWidget *toPageWidget = targetStackedWidget->widget(toPageIndex);
    if (fromPageWidget == nullptr || toPageWidget == nullptr) {
        return false;
    }

    isAnimationRunning = true;

    QParallelAnimationGroup *animationGroup = new QParallelAnimationGroup(this);
    int widgetWidth = stackedWidgetWidth;
    int widgetHeight = stackedWidgetHeight;
    const int animationDuration = 150; // 动画时长

    // 创建两个动画对象
    QPropertyAnimation *fromPageAnimation = new QPropertyAnimation(fromPageWidget, "pos");
    QPropertyAnimation *toPageAnimation = new QPropertyAnimation(toPageWidget, "pos");

    // 设置动画公共属性
    fromPageAnimation->setDuration(animationDuration);
    toPageAnimation->setDuration(animationDuration);
    fromPageAnimation->setEasingCurve(QEasingCurve::OutSine);
    toPageAnimation->setEasingCurve(QEasingCurve::OutSine);

    // 根据方向设置动画终点（用switch，小白易懂）
    switch (direction) {
    case SlideLeftToRight: // 左→右
        toPageAnimation->setEndValue(QPoint(0, 0));
        fromPageAnimation->setEndValue(QPoint(widgetWidth, 0));
        break;
    case SlideRightToLeft: // 右→左
        toPageAnimation->setEndValue(QPoint(0, 0));
        fromPageAnimation->setEndValue(QPoint(-widgetWidth, 0));
        break;
    case SlideUpToDown: // 上→下
        toPageAnimation->setEndValue(QPoint(0, 0));
        fromPageAnimation->setEndValue(QPoint(0, widgetHeight));
        break;
    case SlideDownToUp: // 下→上
        toPageAnimation->setEndValue(QPoint(0, 0));
        fromPageAnimation->setEndValue(QPoint(0, -widgetHeight));
        break;
    }

    // 添加动画到组
    animationGroup->addAnimation(fromPageAnimation);
    animationGroup->addAnimation(toPageAnimation);

    // 保存动画控件
    animationFromWidget = fromPageWidget;
    animationToWidget = toPageWidget;

    // 连接结束信号
    connect(animationGroup, &QParallelAnimationGroup::finished, this, &Gesture::onSwitchAnimationFinished);
    animationGroup->start(QAbstractAnimation::DeleteWhenStopped);
    return true;
}

// 切页动画结束处理
void Gesture::onSwitchAnimationFinished()
{
    // 空指针检查
    if (animationFromWidget == nullptr || animationToWidget == nullptr) {
        return;
    }

    // 隐藏起始页面，恢复位置
    animationFromWidget->hide();
    animationFromWidget->move(0, 0);
    // 设置目标页面为当前页面
    targetStackedWidget->setCurrentWidget(animationToWidget);
    // 重置动画状态
    isAnimationRunning = false;
    animationFromWidget = nullptr;
    animationToWidget = nullptr;
}

// 打开页面
void Gesture::openApp(int pageIndex)
{
    // 正在动画或页面相同，返回
    if (isAnimationRunning == true || pageIndex == targetStackedWidget->currentIndex()) {
        return;
    }

    isAnimationRunning = true;
    // 显示屏蔽控件（防止操作）
    animationBlockerWidget->show();
    animationBlockerWidget->raise();

    // 获取目标页面控件
    QWidget *targetPageWidget = targetStackedWidget->widget(pageIndex);
    // 截图（简化，小白易懂）
    QPixmap pageScreenshot = targetPageWidget->grab(QRect(0, 0, stackedWidgetWidth, stackedWidgetHeight));
    // 快速缩放（减少性能消耗）
    pageScreenshot = pageScreenshot.scaled(stackedWidgetWidth/8, stackedWidgetHeight/8, Qt::KeepAspectRatio, Qt::FastTransformation);

    // 创建缩放动画的覆盖标签
    animationOverlayLabel = new QLabel(targetStackedWidget);
    // 设置标签属性（减少绘制开销）
    animationOverlayLabel->setAttribute(Qt::WA_TranslucentBackground);
    animationOverlayLabel->setPixmap(pageScreenshot);
    animationOverlayLabel->setScaledContents(true);
    // 设置初始位置（小矩形）
    animationOverlayLabel->setGeometry(calculateSmallRect(targetStackedWidget->rect()));
    animationOverlayLabel->show();
    animationOverlayLabel->raise();

    // 创建缩放动画
    QPropertyAnimation *scaleAnimation = new QPropertyAnimation(animationOverlayLabel, "geometry");
    scaleAnimation->setDuration(350); // 动画时长350ms
    scaleAnimation->setStartValue(calculateSmallRect(targetStackedWidget->rect()));
    scaleAnimation->setEndValue(targetStackedWidget->rect());
    scaleAnimation->setEasingCurve(QEasingCurve::OutCubic); // 缓动曲线（更自然）

    // 保存目标页面
    animationToWidget = targetPageWidget;
    // 连接结束信号
    connect(scaleAnimation, &QPropertyAnimation::finished, this, &Gesture::onOpenAppAnimationFinished);
    scaleAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

// 打开页面动画结束
void Gesture::onOpenAppAnimationFinished()
{
    // 设置目标页面为当前页面
    targetStackedWidget->setCurrentWidget(animationToWidget);
    // 释放覆盖标签
    if (animationOverlayLabel != nullptr) {
        animationOverlayLabel->deleteLater();
        animationOverlayLabel = nullptr;
    }
    // 隐藏屏蔽控件
    animationBlockerWidget->hide();
    // 重置动画状态
    isAnimationRunning = false;
}

// 关闭页面
void Gesture::closeApp(int pageIndex)
{
    if (isAnimationRunning == true || pageIndex == targetStackedWidget->currentIndex()) {
        return;
    }

    isAnimationRunning = true;

    // 获取当前页面和目标页面
    QWidget *currentPageWidget = targetStackedWidget->currentWidget();
    QWidget *targetPageWidget = targetStackedWidget->widget(pageIndex);

    // 显示屏蔽控件
    animationBlockerWidget->show();
    animationBlockerWidget->raise();

    // 截图当前页面
    QPixmap pageScreenshot = currentPageWidget->grab(QRect(0, 0, stackedWidgetWidth, stackedWidgetHeight));
    pageScreenshot = pageScreenshot.scaled(stackedWidgetWidth/8, stackedWidgetHeight/8, Qt::KeepAspectRatio, Qt::FastTransformation);

    // 创建覆盖标签
    animationOverlayLabel = new QLabel(targetStackedWidget);
    animationOverlayLabel->setAttribute(Qt::WA_TranslucentBackground);
    animationOverlayLabel->setPixmap(pageScreenshot);
    animationOverlayLabel->setScaledContents(true);
    animationOverlayLabel->setGeometry(targetStackedWidget->rect());
    animationOverlayLabel->show();
    animationOverlayLabel->raise();

    // 隐藏当前页面
    currentPageWidget->hide();

    // 创建缩放动画（缩小）
    QPropertyAnimation *scaleAnimation = new QPropertyAnimation(animationOverlayLabel, "geometry");
    scaleAnimation->setDuration(350);
    scaleAnimation->setStartValue(targetStackedWidget->rect());
    QRect r = targetStackedWidget->rect();
    QPoint center = r.center();

    scaleAnimation->setEndValue(QRect(center.x(), center.y(), 1, 1));
    scaleAnimation->setEasingCurve(QEasingCurve::InCubic);

    // 保存目标页面
    animationToWidget = targetPageWidget;
    // 连接结束信号
    connect(scaleAnimation, &QPropertyAnimation::finished, this, &Gesture::onCloseAppAnimationFinished);
    scaleAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

// 关闭页面动画结束
void Gesture::onCloseAppAnimationFinished()
{
    if (animationToWidget == nullptr) {
        return;
    }

    // 设置目标页面为当前页面并显示
    targetStackedWidget->setCurrentWidget(animationToWidget);
    targetStackedWidget->currentWidget()->show();

    // 释放覆盖标签
    if (animationOverlayLabel != nullptr) {
        animationOverlayLabel->deleteLater();
        animationOverlayLabel = nullptr;
    }
    // 隐藏屏蔽控件
    animationBlockerWidget->hide();
    // 重置动画状态
    isAnimationRunning = false;
}

// 从左往右滑页
void Gesture::slidePageLeftToRight(int pageIndex)
{
    if (targetStackedWidget == nullptr || isAnimationRunning == true) {
        return;
    }

    int currentPageIndex = targetStackedWidget->currentIndex();
    if (currentPageIndex == pageIndex) {
        return;
    }

    QWidget *targetPageWidget = targetStackedWidget->widget(pageIndex);
    int widgetWidth = stackedWidgetWidth;

    // 初始位置在左侧
    targetPageWidget->setGeometry(targetStackedWidget->rect());
    targetPageWidget->move(-widgetWidth, 0);
    targetPageWidget->show();
    targetPageWidget->raise();

    // 启动带方向的动画
    startSwitchAnimation(currentPageIndex, pageIndex, SlideLeftToRight);
}

// 从右往左滑页
void Gesture::slidePageRightToLeft(int pageIndex)
{
    if (targetStackedWidget == nullptr || isAnimationRunning == true) {
        return;
    }

    int currentPageIndex = targetStackedWidget->currentIndex();
    if (currentPageIndex == pageIndex) {
        return;
    }

    QWidget *targetPageWidget = targetStackedWidget->widget(pageIndex);
    int widgetWidth = stackedWidgetWidth;

    // 初始位置在右侧
    targetPageWidget->setGeometry(targetStackedWidget->rect());
    targetPageWidget->move(widgetWidth, 0);
    targetPageWidget->show();
    targetPageWidget->raise();

    // 启动带方向的动画
    startSwitchAnimation(currentPageIndex, pageIndex, SlideRightToLeft);
}

// 从上到下滑页
void Gesture::slidePageUpToDown(int pageIndex)
{
    if (targetStackedWidget == nullptr || isAnimationRunning == true) {
        return;
    }

    int currentPageIndex = targetStackedWidget->currentIndex();
    if (currentPageIndex == pageIndex) {
        return;
    }

    QWidget *targetPageWidget = targetStackedWidget->widget(pageIndex);
    int widgetHeight = stackedWidgetHeight;

    // 初始位置在上侧
    targetPageWidget->setGeometry(targetStackedWidget->rect());
    targetPageWidget->move(0, -widgetHeight);
    targetPageWidget->show();
    targetPageWidget->raise();

    // 启动带方向的动画
    startSwitchAnimation(currentPageIndex, pageIndex, SlideUpToDown);
}

// 从下到上滑页
void Gesture::slidePageDownToUp(int pageIndex)
{
    if (targetStackedWidget == nullptr || isAnimationRunning == true) {
        return;
    }

    int currentPageIndex = targetStackedWidget->currentIndex();
    if (currentPageIndex == pageIndex) {
        return;
    }

    QWidget *targetPageWidget = targetStackedWidget->widget(pageIndex);
    int widgetHeight = stackedWidgetHeight;

    // 初始位置在下侧
    targetPageWidget->setGeometry(targetStackedWidget->rect());
    targetPageWidget->move(0, widgetHeight);
    targetPageWidget->show();
    targetPageWidget->raise();

    // 启动带方向的动画
    startSwitchAnimation(currentPageIndex, pageIndex, SlideDownToUp);
}
