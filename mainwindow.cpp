#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //ui->tableWidget->horizontalHeader()->setVisible(false);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setShowGrid(false);

    // Declare timer as a member variable of MainWindow
    QTimer* timer = new QTimer(this);

    // Connect the timeout signal of the timer to the refreshPing slot
    QObject::connect(timer, &QTimer::timeout, this, &MainWindow::refreshPing);

    // Set the interval to one minute (60,000 milliseconds)
    timer->setInterval(20000);

    // Start the timer
    timer->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

MainWindow::PingResult MainWindow::ping(const QString &host)
{
    QProcess process;
    QString command = "ping " + host;
    process.start("cmd", QStringList() << "/c" << command);
    process.waitForFinished(300);

    MainWindow::PingResult result;

    if (process.exitCode() == 0) {
        result.success = true;

        QString output = process.readAllStandardOutput();
        qDebug() << "output: " << output;

        // Use regular expressions to extract information from the output
        static const QRegularExpression statusRegex("Respuesta desde .*: bytes=(\\d+) tiempo=(\\d+)ms TTL=(\\d+)");

        QRegularExpressionMatchIterator statusMatches = statusRegex.globalMatch(output);

        while (statusMatches.hasNext()) {
            QRegularExpressionMatch match = statusMatches.next();

            result.status = "Ok";
            result.time = match.captured(2) + "ms";
        }

    } else {
        result.success = false;
        result.status = process.errorString();
    }

    return result;
}

QVector<MainWindow::Ping> pingData;

void MainWindow::on_addBtn_clicked()
{
    ui->tableWidget->setRowCount(pingData.count());

    QString hostIp = MainWindow::getHostIp(ui->addLT->text());

    if(hostIp.isEmpty()){
        qDebug() << "Error with hostIp, empty string";
    }

    MainWindow::PingResult result = MainWindow::ping(ui->addLT->text());

    if (result.success) {
        qDebug() << "Time:" << result.time;
        pingData.push_back(MainWindow::Ping{pingId + 1, ui->addLT->text(), hostIp, result.time, "OK"});
        pingId = pingId + 1;

        MainWindow::updateTable();
    } else {
        qDebug() << "Ping Error:" << result.status;
    }
}

void MainWindow::updateTable()
{
    ui->tableWidget->setRowCount(pingData.count());

    for (int i = 0; i < pingData.count(); ++i) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(pingData[i].id)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(pingData[i].host));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(pingData[i].ip));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(pingData[i].time));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(pingData[i].status));
    }
}

QString MainWindow::getHostIp(const QString &hostname) {
    // Resolve the IP address of the host using QHostInfo
    QHostInfo hostInfo = QHostInfo::fromName(hostname);
    if (hostInfo.error() != QHostInfo::NoError) {
        qDebug() << "Error resolving host:" << hostInfo.errorString();
        return QString();
    }

    for (const QHostAddress &address : hostInfo.addresses()) {
        qDebug() << "Resolved IP:" << address.toString();
        return address.toString();
    }

    return hostInfo.errorString();
}

void MainWindow::checkPingStatus(){
    for (int i = 0; i < pingData.count(); ++i) {
        MainWindow::PingResult result = MainWindow::ping(pingData[i].host);

        pingData[i].time = result.time;
        pingData[i].status = result.status;
    }

    MainWindow::updateTable();
}

void MainWindow::refreshPing(){
    qDebug() << "refreshPing";
    MainWindow::checkPingStatus();
}
