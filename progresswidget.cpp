#include "progresswidget.hpp"

ProgressWidget::ProgressWidget(QWidget *parent) : QWidget(parent) {
  layout = new QVBoxLayout;
  
  label = new QLabel;
  
  bar = new QProgressBar;
  
  layout->addWidget(label);
  layout->addWidget(bar);
  
  setLayout(layout);
  show();
}
