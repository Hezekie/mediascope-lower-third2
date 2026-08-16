#pragma once

#include <QWidget>

QWidget *widget_create_tip_card_a(QWidget *parent);
QWidget *widget_create_tip_card_b(QWidget *parent);
QWidget *create_widget_carousel(QWidget *parent);

// Opens a richer help dialog (used by the dock info button).
void show_troubleshooting_dialog(QWidget *parent);
