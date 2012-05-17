#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>

class ProgressWidget : public QWidget {
  Q_OBJECT
  
  private:
    QLabel *label;
    QProgressBar *bar;
    QVBoxLayout *layout;
    
  public:
    ProgressWidget(QWidget *parent=0);
    
};
