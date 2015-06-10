#include "mainwindow.h"
#include <QCoreApplication>
#include <QApplication>
#include <QStringList>

#define xfourcc(a,b,c,d)\
        (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),out(stdout)
{
    mainLayout = new QVBoxLayout();
    colorVid = new VideoWidget();
    colorVid->setWindowTitle("Color");
    colorVid->show();
    depthVid = new VideoWidget();
    depthVid->setWindowTitle("Depth");
    depthVid->show();
    irVid = new VideoWidget();
    irVid->setWindowTitle("Infrared");
    irVid->show();
    colorCamera = new CameraDataFeed();
    colorCamera->setObjectName("color");
    colorCamera->setFourcc(0x56595559);
    depthCamera = new CameraDataFeed();
    depthCamera->setObjectName("depth");
    depthCamera->setFourcc(xfourcc('I','N','R','I'));
    controlsWidget = new ControlsWidget();
    controlsWidget->setCamera(depthCamera);
    controlsWidget->setWindowTitle("Video Controls");
    controlsWidget->show();

    devicePickLayout = new QVBoxLayout();
    colorDevicePickLayout = new QHBoxLayout();
    colorDeviceLabel = new QLabel("Color Device");
    colorDevicePathEdit = new QLineEdit();
    colorDeviceLabel->setBuddy(colorDevicePathEdit);
    colorDevicePickLayout->addWidget(colorDeviceLabel);
    colorDevicePickLayout->addWidget(colorDevicePathEdit);

    depthDevicePickLayout = new QHBoxLayout();
    depthDeviceLabel = new QLabel("Depth Device");
    depthDevicePathEdit = new QLineEdit();
    depthDeviceLabel->setBuddy(depthDevicePathEdit);
    depthDevicePickLayout->addWidget(depthDeviceLabel);
    depthDevicePickLayout->addWidget(depthDevicePathEdit);
    devicePickLayout->addLayout(colorDevicePickLayout);
    devicePickLayout->addLayout(depthDevicePickLayout);

    snapshotLayout = new QHBoxLayout();
    snapshotDirEditLabel = new QLabel("Snapshot Directory (/tmp/depthview)");
    snapshotDirEdit = new QLineEdit();
    snapshotDirEditLabel->setBuddy(snapshotDirEdit);
    snapshotButton = new QPushButton("save picure");
    snapshotLayout->addWidget(snapshotDirEditLabel);
    snapshotLayout->addWidget(snapshotDirEdit);
    snapshotLayout->addWidget(snapshotButton);

    fifoRemoteLayout = new QHBoxLayout();
    fifoRemoteFilenameLabel = new QLabel("fifo trigger filename (/tmp/depthview/fifo)");
    fifoRemoteFilenameEdit = new QLineEdit();
    fifoRemoteFilenameLabel->setBuddy(fifoRemoteFilenameEdit);
    fifoRemoteLayout->addWidget(fifoRemoteFilenameLabel);
    fifoRemoteLayout->addWidget(fifoRemoteFilenameEdit);

    buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("start");
    stopButton = new QPushButton("stop");
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);

    /*minLayout = new QHBoxLayout();
    minLabel = new QLabel("Min Distance");
    minSetting = new QSlider(Qt::Horizontal);
    minSetting->setMinimum(0);
    minSetting->setMaximum(0xffff);
    minSetting->setValue(0);
    minSetting->setSingleStep(1);
    minSetting->setTickInterval(2048);
    minSetting->setTickPosition(QSlider::TicksBothSides);
    minLayout->addWidget(minLabel);
    minLayout->addWidget(minSetting);

    maxLayout = new QHBoxLayout();
    maxLabel = new QLabel("Max Distance");
    maxSetting = new QSlider(Qt::Horizontal);
    maxSetting->setMinimum(0);
    maxSetting->setMaximum(0xffff);
    maxSetting->setValue(0xffff);
    maxSetting->setSingleStep(1);
    maxSetting->setTickInterval(2048);
    maxSetting->setTickPosition(QSlider::TicksBothSides);
    maxLayout->addWidget(maxLabel);
    maxLayout->addWidget(maxSetting);

    byteLayout = new QHBoxLayout();
    byteGroup = new QButtonGroup();
    bothButton = new QRadioButton("both bytes");
    lowButton = new QRadioButton("low byte");
    highButton = new QRadioButton("high byte");
    byteGroup->addButton(bothButton,0xffff);
    byteGroup->addButton(lowButton,0x00ff);
    byteGroup->addButton(highButton,0xff00);
    bothButton->setChecked(true);
    byteLayout->addWidget(bothButton);
    byteLayout->addWidget(lowButton);
    byteLayout->addWidget(highButton);*/

    /*mainLayout->addLayout(minLayout);
    mainLayout->addLayout(maxLayout);
    mainLayout->addLayout(byteLayout);*/
    mainLayout->addLayout(snapshotLayout);
    mainLayout->addLayout(fifoRemoteLayout);

    mainLayout->addLayout((devicePickLayout));
    mainLayout->addLayout(buttonLayout);
    centerWidget = new QWidget();
    centerWidget->setLayout(mainLayout);
    setCentralWidget(centerWidget);

    timer = new QTimer(this);
    fifo_fd = -1;

    connect(colorCamera,SIGNAL(newDepthImage(QImage)),depthVid,SLOT(setImage(QImage)));
    connect(colorCamera, SIGNAL(newInfraredImage(QImage)), irVid, SLOT(setImage(QImage)));
    connect(stopButton,SIGNAL(clicked()), colorCamera, SLOT(stopVideo()));
    connect(colorDevicePathEdit,SIGNAL(textChanged(QString)),colorCamera,SLOT(setCameraDevice(QString)));

    connect(colorCamera,SIGNAL(newColorImage(QImage)),colorVid,SLOT(setImage(QImage)));
    connect(depthCamera,SIGNAL(newDepthImage(QImage)),depthVid,SLOT(setImage(QImage)));
    connect(depthCamera, SIGNAL(newInfraredImage(QImage)), irVid, SLOT(setImage(QImage)));
    connect(stopButton,SIGNAL(clicked()), depthCamera, SLOT(stopVideo()));
    connect(depthDevicePathEdit,SIGNAL(textChanged(QString)),depthCamera,SLOT(setCameraDevice(QString)));

    connect(startButton, SIGNAL(clicked()), colorCamera, SLOT(startVideo()));
    connect(startButton, SIGNAL(clicked()), depthCamera, SLOT(startVideo()));
    connect(startButton,SIGNAL(clicked()),this,SLOT(openFifo()));

    connect(stopButton,SIGNAL(clicked()),this,SLOT(closeFifo()));

    /*
    connect(minSetting,SIGNAL(valueChanged(int)),depthCamera,SLOT(setDepthMin(int)));
    connect(maxSetting,SIGNAL(valueChanged(int)),depthCamera,SLOT(setDepthMax(int)));
    connect(byteGroup,SIGNAL(buttonClicked(int)),depthCamera,SLOT(setDepthMask(int)));*/

    connect(snapshotButton,SIGNAL(clicked()),this,SLOT(ontakeSnap()));
    connect(this,SIGNAL(takeSnap(QDateTime)),colorCamera,SLOT(savePicture(QDateTime)));
    connect(this,SIGNAL(takeSnap(QDateTime)),depthCamera,SLOT(savePicture(QDateTime)));

    connect(snapshotDirEdit,SIGNAL(textChanged(QString)),colorCamera,SLOT(setSnapshotDir(QString)));
    connect(snapshotDirEdit,SIGNAL(textChanged(QString)),depthCamera,SLOT(setSnapshotDir(QString)));
    connect(fifoRemoteFilenameEdit,SIGNAL(textChanged(QString)),
            this,SLOT(setFifoFilename(QString)));

    settings = new QSettings("solsticlipse", "depthview");
    if(settings->contains("colorCameraDevice")){
        colorDevicePathEdit->setText(settings->value("colorCameraDevice").toString());
    }
    if(settings->contains("depthCameraDevice")){
        depthDevicePathEdit->setText(settings->value("depthCameraDevice").toString());
    }
    if(settings->contains("snapshotDirectory")){
        snapshotDirEdit->setText(settings->value("snapshotDirectory").toString());
    }
    if(settings->contains("fifoRemoteFilename")){
        fifoRemoteFilenameEdit->setText(settings->value("fifoRemoteFilename").toString());
    }
    connect(timer, SIGNAL(timeout()), this, SLOT(checkFifo()));

    QStringList cmdline_args = QCoreApplication::arguments();
    if(cmdline_args.size() > 1)
    {
        if(cmdline_args.at(1) == "start")
        {
            out << "start!"<<endl;
            QApplication::postEvent(startButton,
                                       new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, 0, 0));
               QApplication::postEvent(startButton,
                                       new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, 0, 0));
        }
    }
}


MainWindow::~MainWindow()
{
    delete timer;
    settings->setValue("colorCameraDevice",colorDevicePathEdit->text());
    settings->setValue("depthCameraDevice",depthDevicePathEdit->text());
    settings->setValue("snapshotDirectory",snapshotDirEdit->text());
    settings->setValue("fifoRemoteFilename",fifoRemoteFilenameEdit->text());
    delete settings;
}

void MainWindow::setFifoFilename(QString filename){
    fifo_filename = filename;
}

void MainWindow::openFifo(){
    if(!fifo_filename.isEmpty())
    {
            out << "FIFO" << endl;
            fifo_fd = open(fifo_filename.toStdString().c_str(), O_NONBLOCK | O_RDWR);
            if (fifo_fd == -1)
            {
                out << " error opening fifo " << fifo_filename << endl;
                return;
            }
    }
    timer->start(1);
    return;
}

void MainWindow::ontakeSnap()
{
    emit takeSnap(QDateTime::currentDateTime());
}

void MainWindow::closeFifo(){
    timer->stop();
    if (-1 == ::close(fifo_fd)){
        perror("close fifo");
    }
    fifo_fd = -1;
}

void MainWindow::checkFifo(){
    char command;
    if (-1 == read(fifo_fd,&command,1)){
        if(errno == EAGAIN){
            /* no command */
        } else if(errno == ENODEV){
            /* no fifo */
        }
    } else {
        out << "takeSnap" << endl;
        emit takeSnap(QDateTime::currentDateTime());
    }
}
