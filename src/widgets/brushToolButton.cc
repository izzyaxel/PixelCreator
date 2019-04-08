#include "brushToolButton.hh"

#include "../util/globals.hh"

BrushToolButton::BrushToolButton(QString name, QWidget *parent) : QPushButton(name, parent)
{
	this->connect(this, SIGNAL(clicked()), this, SLOT(selectTool()));
}

void BrushToolButton::selectTool()
{
	State::tool = Tools::BRUSH;
}