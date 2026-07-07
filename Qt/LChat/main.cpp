#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //a.setPalette(a.style()->standardPalette());//浅色主题

    //设置全局QSS样式表
    QFile qss(":/style/stylesheet.qss");// :/ 表示路径指向 QT 资源文件
    if(qss.open(QFile::ReadOnly)){
        //打开成功
        qDebug("Open success!");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }
    else{
        //打开失败
        qDebug("Open failed!");
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "LChat_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    //读取配置文件
    QString app_path = QCoreApplication::applicationDirPath();// 获取当前应用程序的路径
    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                                                   QDir::separator() + fileName);//QDir::toNativeSeparators() 将路径分隔符转换为当前操作系统的格式（Windows用\，Linux/macOS用/）
    //创建 QSettings 对象来读取INI格式的配置文件
    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();//获取服务器地址
    QString gate_port = settings.value("GateServer/port").toString();//获取服务器端口
    //拼接成完整的HTTP URL
    gate_url_prefix = "http://"+gate_host+":"+gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
