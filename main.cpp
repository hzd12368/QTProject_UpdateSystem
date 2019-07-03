#include "UpdateSystem.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);    
    Prj_Path = argv[0]; //必须放在Mainwindows之前，绝对路径
    int LastPrjName1 = Prj_Path.lastIndexOf('\\');
    int LastPrjName2 = Prj_Path.lastIndexOf('.');
    Prj_Name = Prj_Path.mid(LastPrjName1 + 1, LastPrjName2 - LastPrjName1 - 1); //不带后缀

    UpdateSystem w;
    w.show();
    return a.exec();
}
