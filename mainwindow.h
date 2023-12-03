#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    struct Ping{
        int id;
        QString host;
        QString ip;
        QString time;
        QString status;
    };
    struct PingResult {
        bool success;      // Indicates whether the ping was successful
        QString status;    // Status information (e.g., "Respuesta desde ...")
        QString time;      // Round-trip time (e.g., "23ms")
        QString packets;   // Packet statistics (e.g., "Paquetes: enviados = 4, recibidos = 4, perdidos = 0 (0% perdidos)")
        QString timings;   // Timings information (e.g., "Mínimo = 19ms, Máximo = 23ms, Media = 21ms")

    };
    PingResult ping(const QString &host);
    int pingId = 0;


private slots:
    void on_addBtn_clicked();
    void updateTable();
    QString getHostIp(const QString &hostname);
    void checkPingStatus();
    void refreshPing();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
