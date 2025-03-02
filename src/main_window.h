/**
 * Image Annotation Tool for image annotations with pixelwise masks
 *
 * Author: Rudra Poudel
 */
#ifndef TestWindow_H
#define TestWindow_H

#include <QMainWindow>
#include <QRectF>
#include <QCloseEvent>
#include <QStringList>
#include <QVector>
#include <QList>
#include <QColor>
#include <QScrollArea>
#include <QAbstractListModel>
#include <QtWidgets/QShortcut>

#include <qstringlistmodel.h>

#include "ui_main_window.h"
#include "image_canvas.h"
#include "label_widget.h"
#include "labels.h"

class MainWindow : public QMainWindow, public Ui::MainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);

private:

    void writeSettings();
    void readSettings();
	void loadConfigLabels();
    void openDirectory();
    QString stringForShortCut(int id) const;
    void registerShortcuts();
	ImageCanvas * newImageCanvas();
	int getImageCanvas(QString name, ImageCanvas *ic) ;
    ImageCanvas * getImageCanvas(int index);
    ImageCanvas * getCurrentImageCanvas();
    ImageMask _tmp;
    QVector<QShortcut*> _shortcuts;
    bool _isLoadingNewLabels;

public:
	ImageCanvas   *  image_canvas ;
    //std::vector<ImageCanvas*> _image_canvas;
	//QScrollArea   *  scroll_area  ;

	Name2Labels      labels       ;
	Id2Labels        id_labels    ;
	QAction        * save_action  ;
	QAction        * set_start_point;
    QAction        * set_end_point;
    QAction        * copy_mask_action;
    QAction        * paste_mask_action;
    QAction        * clear_mask_action;
    QAction        * close_tab_action;
	QAction        * undo_action  ;
    QAction        * swap_action;
    QAction        * redo_action  ;
    QAction        * next_file_action;
	QAction        * previous_file_action;
	QString          curr_open_dir;
public:
	QString currentDir() const;
	QString currentFile() const;
	void updateConnect(const ImageCanvas * ic);
    void allDisconnnect(const ImageCanvas * ic);
    void runWatershed(ImageCanvas * ic);
    void setStarAtNameOfTab(bool star);
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:

	void changeLabel(QListWidgetItem*, QListWidgetItem*);
	void changeColor(QListWidgetItem*);
	void saveConfigFile();
	void loadConfigFile();
	void runWatershed();
    void swapView();
	void on_tree_widget_img_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
	void on_actionOpenDir_triggered();
	//void on_actionOpen_jsq_triggered();
	void on_actionAbout_triggered();
	void closeTab(int index);
    void closeCurrentTab();
    void copyMask();
    void pasteMask();
    void clearMask();
    void nextFile();
    void previousFile();
	void updateConnect(int index);
    void treeWidgetClicked();
    void onLabelShortcut(int row);
    void update();
};

#endif
