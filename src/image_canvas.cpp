
#include "image_canvas.h"
#include "main_window.h"

#include <QtDebug>
#include <QtWidgets>

ImageCanvas::ImageCanvas(MainWindow *ui) :
    QLabel() ,
	_ui(ui){

    _scroll_parent = new QScrollArea(ui);
    setParent(_scroll_parent);
	resize(800,600);
	_scale = _ui->spinbox_scale->value();
	_alpha = _ui->spinbox_alpha->value();
	_pen_size = _ui->spinbox_pen_size->value();
	_initPixmap();
	setScaledContents(true);
	setMouseTracking(true);
	_button_is_pressed = false;

	_undo_list.clear();
	_undo_index = 0;
	_undo = false;

    _scroll_parent->setBackgroundRole(QPalette::Dark);
    _scroll_parent->setWidget(this);

	//set start and end point to 0
	_start_point = QPoint(0, 0);
	_end_point = QPoint(0, 0);

}

ImageCanvas::~ImageCanvas() {
    _scroll_parent->deleteLater();
}

void ImageCanvas::_initPixmap() {
	QPixmap newPixmap = QPixmap(width(), height());
	newPixmap.fill(Qt::white);
	QPainter painter(&newPixmap);
	const QPixmap * p = pixmap();
	if (p != NULL)
		painter.drawPixmap(0, 0, *pixmap());
	painter.end();
	setPixmap(newPixmap);
}

void ImageCanvas::loadImage(const QString &filename) {
	if (!_image.isNull() )
		saveMask();

	_img_file = filename;
	QFileInfo file(_img_file);
	if (!file.exists()) return;

	_image = mat2QImage(cv::imread(_img_file.toStdString()));
	
	_mask_file = file.dir().absolutePath()+ "/" + file.completeBaseName() + "_mask.png";
	_watershed_file = file.dir().absolutePath()+ "/" + file.completeBaseName() + "_watershed_mask.png";

	_watershed = ImageMask(_image.size());
	_undo_list.clear();
	_undo_index = 0;
	if (QFile(_mask_file).exists()) {
		_mask = ImageMask(_mask_file,_ui->id_labels);
        _ui->runWatershed(this);// button_watershed->released());
		_ui->checkbox_manuel_mask->setChecked(true);
		_undo_list.push_back(_mask);
		_undo_index++;
	} else {
		clearMask();
	}
	_ui->undo_action->setEnabled(false);
	_ui->redo_action->setEnabled(false);

	setPixmap(QPixmap::fromImage(_image));
	resize(_scale *_image.size());
}

void ImageCanvas::saveMask() {
	if (isFullZero(_mask.id))
		return;

	_mask.id.save(_mask_file);
	if (!_watershed.id.isNull()) {
        QImage watershed = _watershed.id;
//         if (!_ui->checkbox_border_ws->isChecked()) {
//             watershed = removeBorder(_watershed.id, _ui->id_labels);
//         }
		watershed.save(_watershed_file);
		QFileInfo file(_img_file);
		QString color_file = file.dir().absolutePath() + "/" + file.completeBaseName() + "_color_mask.png";
		idToColor(watershed, _ui->id_labels).save(color_file);
	}
    _undo_list.clear();
    _undo_index = 0;
    _ui->setStarAtNameOfTab(false);
}

// Set start point of the line
void ImageCanvas::setStartPoint() {
	_start_point = _mouse_pos;
	_ui->statusbar->showMessage("Start point set");
}

// Set end point of the line
void ImageCanvas::setEndPoint() {
	_end_point = _mouse_pos;
	_ui->statusbar->showMessage("End point set");
	drawLine();
	emit(_ui->button_watershed->released());
	_start_point = _mouse_pos;
}

void ImageCanvas::scaleChanged(double scale) {
	_scale  = scale ;
	resize(_scale * _image.size());
	
	adjustScrollBars();
	repaint();
}

void ImageCanvas::adjustScrollBars()
{	// from : https://github.com/BestVanRome
	//   x ------>
	//     _________
	// y  |.........|
	// |  |.........|
	// |  |.........|
	// |  |.........|
	// v  |.........|
	//     ---------
	QPointF mPos = _mouse_pos;
	QSize imSize = _act_im_size;


	if (_scroll_parent->verticalScrollBar())
	{
		auto posHeightRel = (mPos.y() / imSize.height());  //Relation Mauspos to Height of Image

		//set vertical scrollbar to mouse position
		QScrollBar* verticalScroll = _scroll_parent->verticalScrollBar();
		double vertScrollSpace = verticalScroll->maximum() - verticalScroll->minimum(); // general calculating of moving-space
		verticalScroll->setValue(vertScrollSpace * posHeightRel);
		//alternative: QWheelEvent::angleDelta().y() -> see example!!! : https://doc.qt.io/qt-5/qwheelevent.html#angleDelta
	}

	if (_scroll_parent->horizontalScrollBar())
	{
		auto posWidthRel = (mPos.x() / imSize.width()); ////Relation Mauspos to Width of Image

		//set horizontal scrollbar to mouse position
		QScrollBar* horizontalScroll = _scroll_parent->horizontalScrollBar();
		double horizScrollSpace = horizontalScroll->maximum() - horizontalScroll->minimum(); // general calculating of moving-space
		horizontalScroll->setValue(horizScrollSpace * posWidthRel);
	}
}

void ImageCanvas::alphaChanged(double alpha) {
	_alpha = alpha;
	repaint();
}

void ImageCanvas::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	QRect rect = painter.viewport();
	QSize size = _scale * _image.size();
	if (size != _image.size()) {
		rect.size().scale(size, Qt::KeepAspectRatio);
		painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
		painter.setWindow(pixmap()->rect());
	}
	painter.drawImage(QPoint(0, 0), _image);
	painter.setOpacity(_alpha);

	if (!_mask.id.isNull() && _ui->checkbox_manuel_mask->isChecked()) {
		painter.drawImage(QPoint(0, 0), _mask.color);
	}
		
	if (!_watershed.id.isNull() && _ui->checkbox_watershed_mask->isChecked()) {
		painter.drawImage(QPoint(0, 0), _watershed.color);
	}

	if (_mouse_pos.x() > 10 && _mouse_pos.y() > 10 && 
		_mouse_pos.x() <= QLabel::size().width()-10 &&
		_mouse_pos.y() <= QLabel::size().height()-10) {
		painter.setBrush(QBrush(_color.color));
		painter.setPen(QPen(QBrush(_color.color), 1.0));
		painter.drawEllipse(_mouse_pos.x() / _scale - _pen_size / 2, _mouse_pos.y() / _scale - _pen_size / 2, _pen_size, _pen_size);
		painter.end();
	}
}

void ImageCanvas::drawLine() {
	QVector<QPoint> points = bresenham(_start_point, _end_point);
	for (int i = 0; i < points.size(); i++) {
		if (_pen_size > 0) {
			int x = points[i].x() / _scale - _pen_size / 2;
			int y = points[i].y() / _scale - _pen_size / 2;
			_mask.drawFillCircle(x, y, _pen_size, _color);
		} else {
			int x = (points[i].x()+0.5) / _scale ;
			int y = (points[i].y()+0.5) / _scale ;
			_mask.drawPixel(x, y, _color);
		}
	}
	repaint();
}

QVector<QPoint> ImageCanvas::bresenham(QPoint _start_point, QPoint _end_point) {
	// Using Bresenham's line algorithm, returns a list of
	// points between two points _start_point and _end_point
	QVector<QPoint> points;
	int x0 = _start_point.x();
	int y0 = _start_point.y();
	int x1 = _end_point.x();
	int y1 = _end_point.y();
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		points.push_back(QPoint(x0, y0));
		if ((x0 == x1) && (y0 == y1)) break;
		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}
	return points;
}

void ImageCanvas::mouseMoveEvent(QMouseEvent * e) {
	_mouse_pos.setX(e->x());
	_mouse_pos.setY(e->y());

	if (_button_is_pressed) 
		_drawFillCircle(e);
	_act_im_size = _image.size() * _scale; //important for adjusting the scrollBars

	//using statusbar to show actual _mouse_pos
	_ui->statusbar->showMessage("X: " + QString::number(_mouse_pos.x()) + " " +
		"Y: " + QString::number(_mouse_pos.y()));
	update();

}

void ImageCanvas::setSizePen(int pen_size) {
	_pen_size = pen_size;
}


void ImageCanvas::mouseReleaseEvent(QMouseEvent * e) {
	if(e->button() == Qt::LeftButton) {
		_button_is_pressed = false;
		
		if (_undo) {
			QMutableListIterator<ImageMask> it(_undo_list);
			int i = 0;
			while (it.hasNext()) {
				it.next();
				if (i++ >= _undo_index)
					it.remove();
			}
			_undo = false;
			_ui->redo_action->setEnabled(false);
		}
		_undo_list.push_back(_mask);
		_undo_index++;
        _ui->setStarAtNameOfTab(true);
		_ui->undo_action->setEnabled(true);
	}

	if (e->button() == Qt::RightButton) { // selection of label
		QColor maskColor = _mask.id.pixel(_mouse_pos / _scale);
		QColor watershedColor = _watershed.id.pixel(_mouse_pos / _scale);
		const LabelInfo * label = _ui->id_labels[maskColor.red()] != NULL? _ui->id_labels[maskColor.red()] : _ui->id_labels[watershedColor.red()];
		if (label != NULL)
		{
			if (!_watershed.id.isNull() && _ui->checkbox_watershed_mask->isChecked()) {
				QColor color = QColor(_watershed.id.pixel(_mouse_pos / _scale));
				QMap<int, const LabelInfo*>::const_iterator it = _ui->id_labels.find(color.red());
				if (it != _ui->id_labels.end()) {
					label = it.value();
				}
			}
			if (label->item != NULL)
				emit(_ui->list_label->currentItemChanged(label->item, NULL));
			refresh();
		}
	}

	if (e->button() == Qt::MiddleButton)
	{
		int x, y;
		if (_pen_size > 0) {
			x = e->x() / _scale;
			y = e->y() / _scale;
		}
		else {
			x = (e->x() + 0.5) / _scale;
			y = (e->y() + 0.5) / _scale;
		}

		_mask.exchangeLabel(x, y, _ui->id_labels, _color);
		update();
	}
}

void ImageCanvas::mousePressEvent(QMouseEvent * e) {
	setFocus();
	if (e->button() == Qt::LeftButton) {
		_button_is_pressed = true;
		_drawFillCircle(e);
	}
}

void ImageCanvas::_drawFillCircle(QMouseEvent * e) {
	if (_pen_size > 0) {
		int x = e->x() / _scale - _pen_size / 2;
		int y = e->y() / _scale - _pen_size / 2;
		_mask.drawFillCircle(x, y, _pen_size, _color);
	} else {
		int x = (e->x()+0.5) / _scale ;
		int y = (e->y()+0.5) / _scale ;
		_mask.drawPixel(x, y, _color);
	}
	update();
}

void ImageCanvas::clearMask() {
	_mask = ImageMask(_image.size());
	_watershed = ImageMask(_image.size());
	_undo_list.clear();
	_undo_index = 0;
	repaint();
	
}

void ImageCanvas::wheelEvent(QWheelEvent * event) {
	int delta = event->delta() > 0 ? 1 : -1;
	if (Qt::ShiftModifier == event->modifiers()) {
        _scroll_parent->verticalScrollBar()->setEnabled(false);
		int value = _ui->spinbox_pen_size->value() + delta * _ui->spinbox_pen_size->singleStep();
		_ui->spinbox_pen_size->setValue(value);
		emit(_ui->spinbox_pen_size->valueChanged(value));
		setSizePen(value);
		repaint();
	} else if (Qt::ControlModifier == event->modifiers()) {
		_scroll_parent->verticalScrollBar()->setEnabled(false);
		double value = _ui->spinbox_scale->value() + delta * _ui->spinbox_scale->singleStep();
		value = std::min<double>(_ui->spinbox_scale->maximum(),value);
		value = std::max<double>(_ui->spinbox_scale->minimum(), value);

		_ui->spinbox_scale->setValue(value);
		scaleChanged(value);
		repaint();
	} else {
        _scroll_parent->verticalScrollBar()->setEnabled(true);
	}
}

void ImageCanvas::keyPressEvent(QKeyEvent * event) {
	if (event->key() == Qt::Key_Space) {
		emit(_ui->button_watershed->released());
	}
}

void ImageCanvas::setWatershedMask(QImage watershed) {
	_watershed.id = watershed;
	idToColor(_watershed.id, _ui->id_labels, &_watershed.color);
}

void ImageCanvas::setMask(const ImageMask & mask) {
	_mask = mask;
}

void ImageCanvas::setActionMask(const ImageMask & mask) {
    setMask(mask);
    _undo_list.push_back(_mask);
    _undo_index++;
    _ui->setStarAtNameOfTab(true);
    _ui->undo_action->setEnabled(true);
}

void ImageCanvas::setId(int id) {
	_color.id = QColor(id, id, id);
	_color.color = _ui->id_labels[id]->color;
}

void ImageCanvas::refresh() {
	if (!_watershed.id.isNull() && _ui->checkbox_watershed_mask->isChecked() ) {
		emit(_ui->button_watershed->released());
	}
	update();
}


void ImageCanvas::undo() {
	_undo = true;
	_undo_index--;
	if (_undo_index == 1) {
		_mask = _undo_list.at(_undo_index - 1);
		_ui->undo_action->setEnabled(false);
		refresh();
	} else if (_undo_index > 1) {
		_mask = _undo_list.at(_undo_index - 1);
		refresh();
	} else {
		_undo_index = 0;
		_ui->undo_action->setEnabled(false);
	}
	_ui->redo_action->setEnabled(true);
}

void ImageCanvas::redo() {
	_undo_index++;
	if (_undo_index < _undo_list.size()) {
		_mask = _undo_list.at(_undo_index - 1);
		refresh();
	}  else if (_undo_index == _undo_list.size()) {
		_mask = _undo_list.at(_undo_index - 1);
		_ui->redo_action->setEnabled(false);
		refresh();
	} else {
		_undo_index = _undo_list.size();
		_ui->redo_action->setEnabled(false);
	}
	_ui->undo_action->setEnabled(true);
}
