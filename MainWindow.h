#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QMap>

class QTabBar;
class QListWidget;
class QListWidgetItem;
class QSplitter;
class QPushButton;
class QComboBox;
class QLineEdit;
class QLabel;
class QWidget;
class QAction;
class QTimer;
class QMenu;

struct Person {
    QString name;
    QMap<QString, QString> information;
};

struct NameList {
    QString filePath;
    QString name;
    QVector<Person> people;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void loadList();
    void saveList();
    void saveListAs();
    void addList();
    void closeList(int index);
    void currentListChanged(int index);
    void personSelected(QListWidgetItem *item);
    void toggleDraw();
    void filterChanged();
    void autoLoadLastListChanged(bool checked);
    void toggleTheme();
    void showAbout();
    void drawStep();

private:
    void setupMenuBar();
    void setupUi();
    void setupConnections();
    void applyTheme();
    void addNameList(const NameList &list);
    void updatePersonList();
    void updatePersonInformation(const Person &person);
    void updateFilterFields();
    void updateDrawCandidates();
    QString serializeList(const NameList &list) const;
    bool deserializeList(const QString &filePath, NameList &list);
    void saveLastPath(const QString &path);
    void loadLastListIfEnabled();

    QMenu *fileMenu = nullptr;
    QMenu *settingsMenu = nullptr;
    QMenu *aboutMenu = nullptr;
    QAction *loadAction = nullptr;
    QAction *saveAction = nullptr;
    QAction *saveAsAction = nullptr;
    QAction *autoLoadAction = nullptr;
    QAction *themeAction = nullptr;
    QAction *aboutAction = nullptr;

    QWidget *centralWidget = nullptr;
    QTabBar *listTabBar = nullptr;
    QPushButton *addListButton = nullptr;
    QSplitter *mainSplitter = nullptr;
    QListWidget *personListWidget = nullptr;
    QLabel *personNameLabel = nullptr;
    QWidget *informationWidget = nullptr;
    QComboBox *filterTypeComboBox = nullptr;
    QLineEdit *filterEdit = nullptr;
    QPushButton *drawButton = nullptr;
    QTimer *drawTimer = nullptr;

    QVector<NameList> nameLists;
    QVector<int> drawCandidates;
    int currentListIndex = -1;
    int currentDrawIndex = -1;
    bool darkTheme = false;
    bool drawing = false;
};

#endif
