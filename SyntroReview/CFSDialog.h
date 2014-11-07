//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroNet
//
//  SyntroNet is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroNet is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef CFSDIALOG_H
#define CFSDIALOG_H

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qlistwidget.h>


class CFSDialog : public QDialog
{
	Q_OBJECT

public:
	CFSDialog(QWidget *parent, QStringList directory);
	
	QStringList newCFSList();

public slots:
	void onAddCFS();
	void onRemoveCFS();
	void onMoveUp();
	void onMoveDown();
	void onCurrentCFSSelectionChanged();
	void onOK();

private:
	void layoutWindow();
	void parseAvailableServices(QStringList directory);

	QStringList m_currentCFSList;
	QStringList m_availableCFSList;

	QListWidget *m_currentList;
	QListWidget *m_availableList;

	QPushButton *m_addButton;
	QPushButton *m_removeButton;
	QPushButton *m_upButton;
	QPushButton *m_downButton;

	QPushButton *m_okButton;
	QPushButton *m_cancelButton;
};

#endif // CFSDIALOG_H
