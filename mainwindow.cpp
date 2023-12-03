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
#include <QFile>
#include <QDir>

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

    MainWindow::readFileToVector("data.txt");

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

        // Save to file
        MainWindow::writeVectorToFile(pingData, "data.txt");
    } else {
        qDebug() << "Ping Error:" << result.status;
    }
}

void MainWindow::updateTable()
{
    ui->tableWidget->setRowCount(pingData.count());

    for (int i = 0; i < pingData.count(); ++i) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(pingData[i].id)));
        ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(pingData[i].host));
        ui->tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(pingData[i].ip));
        ui->tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(pingData[i].time));
        ui->tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(pingData[i].status));
        ui->tableWidget->item(i, 4)->setTextAlignment(Qt::AlignCenter);
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

void MainWindow::writeVectorToFile(const QVector<MainWindow::Ping> &pingData, const QString &fileName) {
    // Get the current application directory path
    QString currentPath = QCoreApplication::applicationDirPath();

    // Combine the current path with the file name
    QString filePath = currentPath + QDir::separator() + fileName;

    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {  // Open the file in text mode
        QTextStream out(&file);

        // Write each struct in the QVector to the file
        for (const MainWindow::Ping &ping : pingData) {
            out << ping.id << ","
            << ping.host << ","
            << ping.ip << ","
            << ping.time << ","
            << ping.status << "\n";

            if (out.status() != QTextStream::Ok) {
                qDebug() << "Error writing to file.";
                file.close();  // Close the file on error
                return;
            }
        }

        // Ensure data is written to the file before closing
        file.close();
        qDebug() << "Saved file:" << filePath;
    } else {
        qDebug() << "Failed to open file for writing:" << filePath;
    }
}

void MainWindow::readFileToVector(const QString &fileName) {
    // Get the current application directory path
    QString currentPath = QCoreApplication::applicationDirPath();

    // Combine the current path with the file name
    QString filePath = currentPath + QDir::separator() + fileName;

    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // Open the file in text mode
        QTextStream in(&file);

        // Read each line from the file and populate the QVector
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split(",");

            if (fields.size() == 5) {  // Ensure the correct number of fields
                MainWindow::Ping ping;
                ping.id = fields[0].toInt();
                ping.host = fields[1];
                ping.ip = fields[2];
                ping.time = fields[3];
                ping.status = fields[4];

                pingData.append(ping);

                MainWindow::updateTable();
            } else {
                qDebug() << "Error reading line from file: " << line;
            }
        }

        // Ensure the file is closed after reading
        file.close();
        qDebug() << "Read file:" << filePath;
    } else {
        qDebug() << "Failed to open file for reading:" << filePath;
    }
}
