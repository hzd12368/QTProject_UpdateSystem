#ifndef UPDATESYSTEM_H
#define UPDATESYSTEM_H

#include <QDialog>
#include <QMessageBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QMovie>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
//JSON相关头文件
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProgressDialog>
#include <QAuthenticator>
#include "ui_UpdateSystem.h"
#include "ui_authenticationdialog.h"
// WinAPI
#include <Windows.h>
#include <winver.h>

extern QString Prj_Path;            // 定义当前软件路径
extern QString Prj_Name;            // 定义当前软件名，不带后缀 （会因为重命名而改变）
extern QString CurVerison;          // 定义当前软件版本号
extern QString Exe_Ver;             // 软件版本
extern QString UpdateSystem_Ver;    // 升级系统版本
extern QString Exe_Name;            // 软件名（不会因为重命名而改变）

struct UpdataInfo{
    QString Verison;
    QString Url;
    QString UpdateTime;
    QString ReleaseNote;
};

namespace Ui {
class UpdateSystem;
}

class UpdateSystem : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateSystem(QWidget *parent = 0);
    ~UpdateSystem();
    void GetUpdateCmd(); //获得升级指令
    void ConnInit();   // 初始化所有的Connnect函数
    void startRequest(QUrl url); //开始下载目的地址获取请求
    QStringList GetFileVer(QString filePath);  // huo

private slots:
    void parse_UpdateJSON(QString str);      //解析JSON文件
    void replyFinished(QNetworkReply *reply);   //网络数据接收完成槽函数的声明
    void VerListSet(UpdataInfo *Info, int size);
    void GetItem(QTableWidgetItem *Item);

    void on_buttonBox_accepted();
    void downloadFile();
    void cancelDownload();
    void httpFinished();
    void httpReadyRead();
    void slotAuthenticationRequired(QNetworkReply*,QAuthenticator *);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply*,const QList<QSslError> &errors);
#endif
    void on_buttonBox_rejected();
    void ToNewPro();

signals:
    StartNewPro();
    CloseAll();
private:
    QProgressDialog *progressDialog;
    QDialogButtonBox *buttonBox;
    QUrl url;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
    QFile *file;
    bool httpRequestAborted;
    Ui::UpdateSystem *ui;
};

class ProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(const QUrl &url, QWidget *parent = nullptr);

public slots:
   void networkReplyProgress(qint64 bytesRead, qint64 totalBytes);
};
#endif // UPDATESYSTEM_H
