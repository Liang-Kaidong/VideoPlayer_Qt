#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QTimer>
#include <QMediaPlayer>     /* 播放器 */
#include <QMediaPlaylist>   /* 播放队列 */
#include <QVideoWidget>     /* 视频显示窗口 */
#include <QButtonGroup>
#include <QListWidgetItem>
#include "class/Gesture/gesture.h"
#include "driver/VoiceControl/voicecontrol.h"

QT_BEGIN_NAMESPACE
namespace Ui { class VideoPlayer; }
QT_END_NAMESPACE

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = nullptr);

    /* 原则上来说，不应该放着，但为了毕设需要，我不想另写函数，偷个懒 */
    QMediaPlayer *mediaPlayer = nullptr;
    QMediaPlaylist *mediaPlayerList = nullptr;
    QVideoWidget *videoWidget = nullptr;

    ~VideoPlayer();

public slots:
    void onMainWindowsVolumeChanged(int value); /* 接收主窗口音量变化（毕设需要，可以删） */
    void resetAPP();    /* 毕设需要，可以删 */

signals:
    void onVideoPlayerVolumeChanged(int value); /* 通知主窗口音量变化（毕设需要，可以删） */

private slots:
    void gotoCurrentPlayPage();

    void loadVideoFile(QString dirPath);

    void on_playListPushButton_clicked();

    void on_backPushButton_clicked();

    void on_chooseFilePathPushButton_clicked();

    void on_controlPlayPushButton_clicked();

    void on_setFullScreenPushButton_clicked();

    void setcontrolWidgetVisible();

    void updateControlPushButtonUI();

    void onDurationChanged(qint64 duration);

    void onPositionChanged(qint64 position);

    void on_nextPushButton_clicked();

    void on_previousPushButton_clicked();

    void on_playHorizontalSlider_sliderPressed();

    void on_playHorizontalSlider_sliderMoved(int position);

    void on_playHorizontalSlider_sliderReleased();

    void onCurrentIndexChanged(int position);

    void on_playSpeedPushButton_clicked();

    void onSpeedButtonClicked(int buttonID);

    void on_volumePushButton_clicked();

    void on_playListWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_volumeVerticalSlider_sliderReleased();

    void on_volumeVerticalSlider_sliderPressed();


private:
    Ui::VideoPlayer *ui;

    bool isFirstEntry = true;
    void showEvent(QShowEvent *event);

    QTimer *firstEntryTimer = nullptr;

    QTimer *setcontrolWidgetVisibleTimer = nullptr;
    bool isFullScreen = false;
    bool eventFilter(QObject *watched, QEvent *event);

    QString originalPlayListPushButtonStyleSheet;
    QString originalPlaySpeedPushButtonStyleSheet;
    QString originalVolumePushButtonStyleSheet;
    QString pushButtonPressedStyleSheet;

    QButtonGroup *speedButtonGroup;     /* 倍速按钮组 */
    QString speedButtonStyleSheet;

    Gesture *gesture;

    VoiceControl *voiceControl;

};
#endif // VIDEOPLAYER_H
