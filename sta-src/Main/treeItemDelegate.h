/*
 This program is free software; you can redistribute it and/or modify it under
 the terms of the European Union Public Licence - EUPL v.1.1 as published by
 the European Commission.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the European Union Public Licence - EUPL v.1.1
 for more details.

 You should have received a copy of the European Union Public Licence - EUPL v.1.1
 along with this program.

 Further information about the European Union Public Licence - EUPL v.1.1 can
 also be found on the world wide web at http://ec.europa.eu/idabc/eupl

*/

/*
 ------ Copyright (C) 2010 STA Steering Board (space.trajectory.analysis AT gmail.com) ----
*/
/*
------------------ Author: Annalisa Riccardi  -------------------------------------------------
 ------------------ E-mail: (nina1983@gmail.com) ----------------------------
 */

#include <QItemDelegate>
#include <QLineEdit>

class TreeItemDelegate : public QItemDelegate{
	public:    
		TreeItemDelegate(QObject* parent = 0) : QItemDelegate(parent) {}     
		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const{
			QWidget* editor = QItemDelegate::createEditor(parent, option, index);    
			if (index.column() == 1)   {
				if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(editor)){ 
					QDoubleValidator* doubleValidator = new QDoubleValidator(editor);
					lineEdit->setValidator(doubleValidator);    
					}
				return editor;  
			}      
			return 0;    
		}
};

