#include "UpdateSystem.h"
#include "qdebug.h"


/* 变量定义区 */
QString Exe_Ver;            // 软件版本
QString Exe_Name;           // 软件名（不会因为重命名而改变）
QString UpdateSystem_Ver;   // 升级系统版本
QString Prj_Path;           // 定义当前软件路径
QString Prj_Name;           // 定义当前软件名，不带后缀（会因重命名而改变）

QString Download_Name;  //下载软件名
int VersionSize = 0; //JSON文件中可用版本个数
QNetworkAccessManager *manager;     //定义JSON请求对象
UpdataInfo TableSet[10]; // 下载所有目标信息
UpdataInfo DownInfo;  //下载目标信息

// 直接获取Github上文件内容格式：https://raw.githubusercontent.com/:owner/:repo/master/:path
//如"https://raw.githubusercontent.com/hzd12368/MySoftwareStack/master/Json/software_update.json";
// 升级所用的Json文件
QString JsonFilePath = "https://raw.githubusercontent.com/hzd12368/MySoftwareStack/master/Json/software_update.json";

UpdateSystem::UpdateSystem(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdateSystem)
{
    ui->setupUi(this);
    QStringList info = GetFileVer(Prj_Path);
    Exe_Ver = info.value(0); //获取本程序软件版本
    UpdateSystem_Ver = info.value(1); //获取升级系统版本
    Exe_Name = info.value(3); //获取升级系统版本
    qDebug() << Exe_Name;
    this->setWindowTitle("软件更新系统 V" + UpdateSystem_Ver);

    manager = new QNetworkAccessManager;          //新建QNetworkAccessManager Json对象
    qDebug() << manager->supportedSchemes();
    //设置Table窗口自适应
    ui->tableWidget_VerList->horizontalHeader()->setAutoScroll(true);
    ui->tableWidget_VerList->horizontalHeader()->setStretchLastSection(true);
    //状态栏设置
    ui->label_Statue->setText(tr("Please choose the version of file to download."));
    ui->label_Statue->setWordWrap(true);

    //buttonbox中ok按钮设为false
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    //去掉help按钮
    Qt::WindowFlags flags= this->windowFlags();
    setWindowFlags(flags&~Qt::WindowContextHelpButtonHint);

#ifndef QT_NO_SSL
    connect(&qnam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
#endif

//    //此方法会将控件中的子控件的背景也设置为透明
//    ui->tableWidget_VerList->setStyleSheet("background-color:rgba(0,0,0,0)");
//    QPalette pll = ui->tableWidget_VerList->palette();
//    pll.setBrush(QPalette::Base,QBrush(QColor(255,255,255,0)));
//    ui->tableWidget_VerList->setPalette(pll);
    ConnInit();  // 初始化Connect链接
    GetUpdateCmd(); //更新升级文件列表

}

UpdateSystem::~UpdateSystem()
{
    delete ui;
}

void UpdateSystem::GetUpdateCmd()
{
    QNetworkRequest quest;
    quest.setUrl(QUrl(JsonFilePath)); //包含最新版本软件的下载地址
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    manager->get(quest);    //发送get网络请求
    qDebug()<< "quest" << JsonFilePath;
}

void UpdateSystem::ConnInit()
{
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));//关联JSON文件信号和槽
    connect(ui->tableWidget_VerList, SIGNAL(itemClicked(QTableWidgetItem*)), this,SLOT(GetItem(QTableWidgetItem*)));// JSON格式写入TableWidget
    connect(&qnam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            this, SLOT(slotAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
#ifndef QT_NO_SSL
    connect(&qnam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
            this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
#endif
    connect(this,SIGNAL(StartNewPro()), this, SLOT(ToNewPro()));
}

void UpdateSystem::startRequest(QUrl url)
{
    httpRequestAborted = false;
    reply = qnam.get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(httpFinished()));
    connect(reply, SIGNAL(readyRead()),this, SLOT(httpReadyRead()));

    //进度条
    ProgressDialog *progressDialog = new ProgressDialog(url, this);
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(progressDialog, &QProgressDialog::canceled, this, &UpdateSystem::cancelDownload);
    connect(reply, &QNetworkReply::downloadProgress, progressDialog, &ProgressDialog::networkReplyProgress);
    connect(reply, &QNetworkReply::finished, progressDialog, &ProgressDialog::hide);
    progressDialog->show();

}

void UpdateSystem::parse_UpdateJSON(QString str)
{
    QJsonParseError err_rpt;
    QJsonDocument  root_Doc = QJsonDocument::fromJson(str.toUtf8(),&err_rpt);//字符串格式化为JSON

    //以QJSONARRY形式进行填充初始化Tablewiget
    if(err_rpt.error != QJsonParseError::NoError)
    {
        QMessageBox::critical(NULL, "检查失败", "资源地址错误或JSON格式错误，请保证网络连接后重试!");
        return;
    }
    if(root_Doc.isObject())
    {
        QJsonObject  root_Obj = root_Doc.object();   //创建JSON对象，不是字符串
        QJsonArray ProjectVersion = root_Obj.value(Exe_Name).toArray(); //名称与工程名称一致
        VersionSize = ProjectVersion.size(); //版本数量
        //获取Json里面的版本信息
        QJsonObject LastVersion = ProjectVersion.at(0).toObject();
        UpdataInfo Last;
        UpdataInfo Old;
        Last.Verison = LastVersion.value("LatestVerison").toString();
        Last.Url = LastVersion.value("Url").toString();
        Last.UpdateTime = LastVersion.value("UpdateTime").toString();
        Last.ReleaseNote = LastVersion.value("ReleaseNote").toString().toUtf8(); // JSON文件格式为UTF-8，转化成中文
        // 写入Table
        TableSet[0] = Last;
        for(int i = 1; i < VersionSize; i++){
            QJsonObject OldVersion = ProjectVersion.at(i).toObject();
            Old.Verison = OldVersion.value("OldVerison").toString();
            Old.Url = OldVersion.value("Url").toString();
            Old.UpdateTime = OldVersion.value("UpdateTime").toString();
            Old.ReleaseNote = OldVersion.value("ReleaseNote").toString().toUtf8(); // JSON文件格式为UTF-8，转化成中文
            TableSet[i] = Old;
        }
        VerListSet(TableSet, ProjectVersion.size());
        // 弹升级提示框
        if(Last.Verison <= ("V" + Exe_Ver)){
            QMessageBox::about(this, "通知", "版本已经是最新的了。您也可以选择旧版本进行降级。");
        }
        else{
            QMessageBox::about(this, "通知", "发现新版本咯，赶紧来升级吧。\r\n"
                                           "注意：升级前请仔细查看更新内容。");
        }
    }
}

void UpdateSystem::replyFinished(QNetworkReply *reply)
{
    QString str = reply->readAll();//读取接收到的数据
    qDebug()<< "str" << str;
    parse_UpdateJSON(str);
    reply->deleteLater();               //销毁请求对象

}

void UpdateSystem::VerListSet(UpdataInfo *Info, int size)
{
    // 增加NEW的gif动画
    QLabel *Lab = new QLabel;
    ui->tableWidget_VerList->setCellWidget(0, 0, Lab);
    QMovie *movie =new QMovie(":/UpdateSystem/images/NewVersion.gif");
    movie->setScaledSize(QSize(Lab->width()/2,Lab->height()));
    Lab->setMovie(movie);
    Lab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);//设置水平靠右，垂直中间
    movie->start();
    //填入表格数据
    for(int i = 0; i < size; i++){
        ui->tableWidget_VerList->setItem(i, 0, new QTableWidgetItem(Info->Verison));
        ui->tableWidget_VerList->setItem(i, 1, new QTableWidgetItem(Info->UpdateTime));
        ui->tableWidget_VerList->setItem(i, 2, new QTableWidgetItem(Info->Url));
        ui->tableWidget_VerList->setItem(i, 3, new QTableWidgetItem(Info->ReleaseNote));
        ui->tableWidget_VerList->resizeRowToContents(i);  // 按内容自动适应行宽
        Info++;
    }
}


void UpdateSystem::GetItem(QTableWidgetItem *Item)
{
    if(VersionSize == 0){ return;} // 如果版本个数为零，返回不显示
    //清除所有背景颜色
    for(int i = 0; i < VersionSize; i++){
       for(int j = 0; j < ui->tableWidget_VerList->columnCount(); j++){
           ui->tableWidget_VerList->item(i, j)->setBackgroundColor(QColor(255,255,255));
        }
    }
    //设置目标行为蓝色背景
    for(int k = 0; k < ui->tableWidget_VerList->columnCount(); k++){
        ui->tableWidget_VerList->item(Item->row(),k)->setBackgroundColor(QColor(0,128,255));
    }
    //获取下载网址    
    DownInfo.Verison = ui->tableWidget_VerList->item(Item->row(),0)->text();
    DownInfo.UpdateTime = ui->tableWidget_VerList->item(Item->row(),1)->text();
    DownInfo.Url = ui->tableWidget_VerList->item(Item->row(),2)->text();
    DownInfo.ReleaseNote = ui->tableWidget_VerList->item(Item->row(),3)->text();
    //将网站显示于状态栏
     if(DownInfo.Url.isEmpty()){
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->label_Statue->setText(tr("Please choose the version of file to download."));
    }
    else{
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->label_Statue->setText(tr("The %1 of file will be download from %2!").arg(DownInfo.Verison).arg(DownInfo.Url));
    }
}

void UpdateSystem::on_buttonBox_accepted()
{
    //将网站显示于状态栏
     if(DownInfo.Url.isEmpty()){
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->label_Statue->setText(tr("Please choose the version of file to download."));
        return;  // 如果DownInfo中的Url为空，不进到downloadFile函数中
    }
    else{
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->label_Statue->setText(tr("The %1 of file will be download from %2!").arg(DownInfo.Verison).arg(DownInfo.Url));
    }
    downloadFile(); //下载文件
}

void UpdateSystem::downloadFile()
{
    url = DownInfo.Url;

    QFileInfo fileInfo(url.path());
    Download_Name = fileInfo.fileName();
    //如果下载的链接中不包含有用信息
    if (Download_Name.isEmpty()){
        Download_Name = "None.html";
    }
    //如果根目录下有相同的文件
    if (QFile::exists(Download_Name)) {
        if (QMessageBox::question(this, tr("HTTP"),
                                  tr("There already exists a file called %1 in "
                                     "the current directory. Overwrite?").arg(Download_Name),
                                  QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
            == QMessageBox::No)
            return;
        QFile::remove(Download_Name);
    }

    file = new QFile(Download_Name);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("HTTP"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(Download_Name).arg(file->errorString()));
        delete file;
        file = 0;
        return;
    }

    ui->buttonBox->setEnabled(false);

    // schedule the request
    startRequest(url);
}

void UpdateSystem::cancelDownload()
{
    ui->label_Statue->setText(tr("Download canceled."));
    httpRequestAborted = true;
    reply->abort();
    ui->buttonBox->setEnabled(true);
}

void UpdateSystem::httpFinished()
{
    if (httpRequestAborted) {
        if (file) {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }
        reply->deleteLater();
        return;
    }
    file->flush();
    file->close();

    if (reply->error()) {
        file->remove();
        QMessageBox::information(this, tr("HTTP"),
                                 tr("Download failed: %1.")
                                 .arg(reply->errorString()));
        ui->buttonBox->setEnabled(true);
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        QUrl newUrl = url.resolved(redirectionTarget.toUrl());
        if (QMessageBox::question(this, tr("HTTP"),
                                  tr("Redirect to %1 ?").arg(newUrl.toString()),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            url = newUrl;
            reply->deleteLater();
            file->open(QIODevice::WriteOnly);
            file->resize(0);
            startRequest(url);
            return;
        }
    }
    else {
        QString fileName = QFileInfo(QUrl(DownInfo.Url).path()).fileName();
        ui->label_Statue->setText(tr("Downloaded %1 to %2 succeed!").arg(fileName).arg(QDir::currentPath()));
        ui->buttonBox->setEnabled(true);
        emit StartNewPro();
    }

    reply->deleteLater();
    reply = 0;
    delete file;
    file = 0;

}

void UpdateSystem::httpReadyRead()
{
    // this slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file){
        file->write(reply->readAll());
    }
}

void UpdateSystem::slotAuthenticationRequired(QNetworkReply*,QAuthenticator *authenticator)
{
    QDialog dlg;
    Ui::Dialog ui;
    ui.setupUi(&dlg);
    dlg.adjustSize();
    ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm()).arg(url.host()));

    // Did the URL have information? Fill the UI
    // This is only relevant if the URL-supplied credentials were wrong
    //输入账号和密码
//    ui.userEdit->setText(url.userName());
//    ui.passwordEdit->setText(url.password());

    if (dlg.exec() == QDialog::Accepted) {
        authenticator->setUser(ui.userEdit->text());
        authenticator->setPassword(ui.passwordEdit->text());
    }
}

#ifndef QT_NO_SSL
void UpdateSystem::sslErrors(QNetworkReply*,const QList<QSslError> &errors)
{
    QString errorString;
    foreach (const QSslError &error, errors) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += error.errorString();
    }

    if (QMessageBox::warning(this, tr("HTTP"),
                             tr("One or more SSL errors has occurred: %1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort) == QMessageBox::Ignore) {
        reply->ignoreSslErrors();
    }
}
#endif

void UpdateSystem::on_buttonBox_rejected()
{
    close();
}

void UpdateSystem::ToNewPro()
{
    switch( QMessageBox::information( this, tr("恭喜"),
                                      tr("新程序已经下载完毕，是否运行新程序？"),
                                      tr("Yes"), tr("No"),
                                      0, 1 ) ){
    case 0:
    {
        //执行新程序
        QProcess::startDetached(Download_Name); //运行新程序，不弹DOS黑框
        //备份原程序
        int BackVerNum = 0;
        QString BackFileName = Prj_Name + "_back_" + QString::number(BackVerNum);
        QFile BackFile(BackFileName);
        // 如果备份文件存在,则名称后缀自加1
        while(BackFile.exists()){
         BackVerNum++;
         BackFileName = Prj_Name + "_back_" + QString::number(BackVerNum);
         BackFile.setFileName(BackFileName); //重新载入备份文件名
        }
        QString CMD = "rename " + Prj_Path  + " " + BackFileName; //重命名
        qDebug() << CMD;
        //执行CMD命令
        char* ptr;
        QByteArray ba = CMD.toLocal8Bit(); //支持含中文
        ptr = ba.data();
        system(ptr); //运行新程序，弹DOS黑框
        emit CloseAll();
        close();
    }
        break;
    case 1:
        break;
    default:
        break;
    }
}

//进度框类
ProgressDialog::ProgressDialog(const QUrl &url, QWidget *parent)
  : QProgressDialog(parent)
{
    setWindowTitle(tr("Download Progress"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setLabelText(tr("Downloading %1.").arg(url.toDisplayString()));
    setMinimum(0);
    setValue(0);
    setMinimumDuration(0);
    setMinimumSize(QSize(400, 75));
}

void ProgressDialog::networkReplyProgress(qint64 bytesRead, qint64 totalBytes)
{
    setMaximum(totalBytes);
    setValue(bytesRead);
}

/**
* This function is used to get the version of file.
* @param[in]   The file Path
* @param[out]  QStringList ret = {FileVersion, ProductVersion, CompanyName, ProductName, OriginalFilename}
* @par Created in 2019-06-04
*/
QStringList UpdateSystem::GetFileVer(QString filePath)
{
    QStringList ret;
    DWORD dwLen = 0;
    char* lpData = NULL;
    BOOL bSuccess = FALSE;
    DWORD vHandle = 0;
    //获得文件基础信息
    //--------------------------------------------------------
    dwLen = GetFileVersionInfoSize(filePath.toStdWString().c_str(), &vHandle);
    if (0 == dwLen)
    {
        qDebug() << "获取版本字节信息失败!";
    }
        qDebug() << "版本信息字节大小：" << dwLen;
    lpData =(char*)malloc(dwLen+1);
    if (NULL == lpData)
    {
        qDebug()<<"分配内存失败";
    }
    bSuccess = GetFileVersionInfo(filePath.toStdWString().c_str(),0, dwLen+1, lpData);
    if (!bSuccess)
    {
        qDebug()<<"获取文件版本信息错误!";
    }
    LPVOID lpBuffer = NULL;
    UINT uLen = 0;

    //获得语言和代码页(language and code page),规定，套用即可
    //---------------------------------------------------
   bSuccess = VerQueryValue( lpData,
                            (TEXT("\\VarFileInfo\\Translation")),
                            &lpBuffer,
                             &uLen);
    QString strTranslation,str1,str2;
    unsigned short int *p =(unsigned short int *)lpBuffer;
    str1.setNum(*p,16);
    str1="000"+ str1;
    strTranslation+= str1.mid(str1.size()-4,4);
    str2.setNum(*(++p),16);
    str2="000"+ str2;
    strTranslation+= str2.mid(str2.size()-4,4);

    QString code;

    //以上步骤需按序进行，以下步骤可根据需要增删或者调整
    //1获得文件版本信息：FileVersion
    //-----------------------------------------------------
    code ="\\StringFileInfo\\"+ strTranslation +"\\FileVersion";
    bSuccess = VerQueryValue(lpData,
                            (code.toStdWString().c_str()),
                            &lpBuffer,
                            &uLen);
    if (!bSuccess)
    {
        ret << "";
        qDebug()<<"获取文件版本信息错误!";
    }
    else
    {
        ret << QString::fromUtf16((const unsigned short int *)lpBuffer);
    }

    //2获得产品版本信息：ProductVersion
    //-----------------------------------------------------
    code ="\\StringFileInfo\\"+ strTranslation +"\\ProductVersion";
    bSuccess = VerQueryValue(lpData,
                            (code.toStdWString().c_str()),
                            &lpBuffer,
                            &uLen);
    if (!bSuccess)
    {
        ret << "";
        qDebug()<<"获取产品版本信息错误!";
    }
    else
    {
        ret << QString::fromUtf16((const unsigned short int *)lpBuffer);
    }

    //3获得公司名 CompanyName
    //---------------------------------------------------------
    code ="\\StringFileInfo\\"+ strTranslation +"\\CompanyName";
    bSuccess = VerQueryValue(lpData,
                            (code.toStdWString().c_str()),
                            &lpBuffer,
                            &uLen);
    if (!bSuccess)
    {
        ret << "";
        qDebug()<<"Get file CompanyName error!";
    }
    else
    {
        ret << QString::fromUtf16((const unsigned short int *)lpBuffer);
    }

    //4获得产品名称
    //---------------------------------------------------------
    code ="\\StringFileInfo\\"+ strTranslation +"\\ProductName";
    bSuccess = VerQueryValue(lpData,
                            (code.toStdWString().c_str()),
                            &lpBuffer,
                            &uLen);
    if (!bSuccess)
    {
        ret << "";
        qDebug()<<"Get file ProductName error!";
    }
    else
    {
        ret << QString::fromUtf16((const unsigned short int *)lpBuffer);
   }

    //获得文件名称 OriginalFilename
    //---------------------------------------------------------
    code ="\\StringFileInfo\\"+ strTranslation +"\\OriginalFilename";
    bSuccess = VerQueryValue(lpData,
                            (code.toStdWString().c_str()),
                            &lpBuffer,
                            &uLen);
    if (!bSuccess)
    {
        ret << "";
        qDebug()<<"Get file OriginalFilename error!";
    }
    else
    {
        ret << QString::fromUtf16((const unsigned short int *)lpBuffer);
   }

    delete lpData;
    return ret;
}

