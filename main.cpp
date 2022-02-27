#include <QApplication>
#include <QPushButton>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QKeyEvent>
#include <cmath>
#include <QLayout>
#include <QLabel>

class Line {
public:
    virtual void move(int dx, int dy) = 0;

    virtual void rotateClockwise() = 0;

    virtual void rotateCounterClockwise() = 0;

    virtual void resize(int delta) = 0;
};

class QtLineAdapter : public Line {
public:
    explicit QtLineAdapter(QGraphicsScene *scene_, QLineF pos) : scene(scene_) {
        line = scene->addLine(pos);
        line->setTransformOriginPoint(pos.x1(), pos.y1());

        p1 = scene->addText("A1");
        p2 = scene->addText("B1");
        p1->setParentItem(line);
        p2->setParentItem(line);

        p1->setPos(line->line().p1());
        p2->setPos(line->line().p2());
    }

    ~QtLineAdapter() {
        scene->removeItem(line);
        scene->removeItem(p1);
        scene->removeItem(p2);
        delete line;
        delete p1;
        delete p2;
    }

    void move(int dx, int dy) override {
        line->moveBy(dx, dy);
    }

    void rotateClockwise() override {
        line->setRotation(line->rotation() + 30);
    }

    void rotateCounterClockwise() override {
        line->setRotation(line->rotation() - 30);
    }

    void resize(int delta) override {
        auto l = line->line();
        l.setLength((l.length() + delta) ?: 1);
        line->setLine(l);
        p2->setPos(l.p2());
    }

private:
    QGraphicsScene *scene;
    QGraphicsLineItem *line;
    QGraphicsItem *p1, *p2;
};

class CustomLine : public Line, public QGraphicsItem {
public:
    explicit CustomLine(QGraphicsScene *scene_, QLineF pos) : QGraphicsItem(), scene(scene_) {
        length = pos.length();
        angle = pos.angle();
        origin = pos.p1();

        scene->addItem(this);
    }

    ~CustomLine() override {
        scene->removeItem(this);
        QGraphicsItem::~QGraphicsItem();
    }

    void move(int dx, int dy) override {
        origin.setX(origin.x() + dx);
        origin.setY(origin.y() + dy);
        update();
    }

    void rotateClockwise() override {
        angle += 30;
        update();
    }

    void rotateCounterClockwise() override {
//        double dx, dy, nx, ny;
//        dx = line.x2() - line.x1();
//        dy = line.y2() - line.y1();
//
//        auto theta = -30 * std::numbers::pi / 180;
//        nx = dx * cos(theta) - dy * sin(theta);
//        ny = dx * sin(theta) + dy * cos(theta);
//        line.setLine(line.x1(), line.y1(), line.x1() + nx, line.y1() + ny);

        angle -= 30;
        update();
    }

    void resize(int delta) override {
        length += delta;
        if (length < 0) {
            length = 0;
        }
        update();
    }

    QRectF boundingRect() const override {
        return scene->sceneRect();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        painter->fillRect(scene->sceneRect(), Qt::white);

        auto oldPen = painter->pen();
        auto pen = oldPen;
        pen.setColor(Qt::red);
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);

        // ACTUALLY DRAW
        auto dx = std::cos(angle * std::numbers::pi / 180) * length;
        auto dy = std::sin(angle * std::numbers::pi / 180) * length;
        drawLine(origin.x(), origin.y(), origin.x() + dx, origin.y() + dy, painter);




        // STOP DRAWING

        painter->setPen(oldPen);
    }

private:
    static void drawLine(qreal x1, qreal y1, qreal x2, qreal y2, QPainter *painter) {
        if (std::abs(y2 - y1) < std::abs(x2 - x1)) {
            if (x1 > x2) {
                drawLineLow(x2, y2, x1, y1, painter);
            } else {
                drawLineLow(x1, y1, x2, y2, painter);
            }
        } else {
            if (y1 > y2) {
                drawLineHigh(x2, y2, x1, y1, painter);
            } else {
                drawLineHigh(x1, y1, x2, y2, painter);
            }
        }

        painter->drawText(int(x1), int(y1) - 5, "A2");
        painter->drawText(int(x2), int(y2) - 5, "B2");
    }

    static void drawLineHigh(qreal x1, qreal y1, qreal x2, qreal y2, QPainter *painter) {
        auto dx = x2 - x1,
            dy = y2 - y1;

        auto sx = dx < 0 ? -1 : 1;
        dx *= sx;

        auto d = 2 * dx - dy;
        auto x = int(x1);

        for (auto y = int(y1); y <= y2; y++) {
            painter->drawPoint(x, y);
            if (d > 0) {
                x += sx;
                d += 2 * (dx - dy);
            } else {
                d += 2 * dx;
            }
        }
    }

    static void drawLineLow(qreal x0, qreal y0, qreal x1, qreal y1, QPainter *painter) {
        auto dx = x1 -x0,
            dy = y1 - y0;
        auto sy = dy < 0? -1 : 1;
        dy = dy * sy;

        auto d = (2 * dy) - dx;
        auto y = y0;
        for (auto x = int(x0); x <= x1; x++){
            painter->drawPoint(x, y);
            if (d > 0){
                y += sy;
                d = d +(2*(dy - dx));
            }
            else{
                d = d+ 2*dy;
            }
        }
    }
    QGraphicsScene *scene;
    QPointF origin;
    qreal length;
    qreal angle;
};

class MainWindow : public QMainWindow {
public:
    MainWindow() : QMainWindow(), scene(), view(&scene), lines(), index() {
        resize(800, 600);

        auto central = new QWidget(this);
        auto layout = new QHBoxLayout(central);
        central->setLayout(layout);
        setCentralWidget(central);

        auto label = new QLabel("Controls:\n1. Movement: WASD\n2. Rotation: QE\n3. Scale: ZC\n4. Switch: Space", central);
        layout->addWidget(label);
        layout->addWidget(&view);

        scene.setParent(central);
        view.setParent(central);

        scene.setSceneRect(0, 0, 550, 550);
        view.setSceneRect(0, 0, 550, 550);

        lines[1] = new CustomLine(&scene, {{100, 200, 200, 200}});
        lines[0] = new QtLineAdapter(&scene, {{100, 100},
                                              {200, 100}});
    }

    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
            case Qt::Key_Space:
                index = !index;
                break;

            case Qt::Key_W:
                lines[index]->move(0, -100);
                break;
            case Qt::Key_S:
                lines[index]->move(0, 100);
                break;
            case Qt::Key_A:
                lines[index]->move(-100, 0);
                break;
            case Qt::Key_D:
                lines[index]->move(100, 0);
                break;
            case Qt::Key_Q:
                lines[index]->rotateCounterClockwise();
                break;
            case Qt::Key_E:
                lines[index]->rotateClockwise();
                break;
            case Qt::Key_Z:
                lines[index]->resize(-10);
                break;
            case Qt::Key_C:
                lines[index]->resize(10);
                break;
        }
    }

private:
    QGraphicsScene scene;
    QGraphicsView view;
    Line *lines[2];
    bool index;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow mainWindow;

    mainWindow.show();

    return QApplication::exec();
}
