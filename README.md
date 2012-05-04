osmwidget
=========

A QT Widget for displaying Open Street Map data

building
========

You need qmake for QT4 (called qmake-qt4 on some systems).

qmake -profile
qmake

You need to edit osmwidget.pro and add two lines after the initial defines:
QT += xml
QT += sql

Finally, you can run

make
