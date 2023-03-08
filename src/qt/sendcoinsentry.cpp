#include "sendcoinsentry.h"
#include "ui_sendcoinsentry.h"
#include "guiutil.h"
#include "altcoinunits.h"
#include "addressbookpage.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"

#include <QApplication>
#include <QClipboard>

SendCoinsEntry::SendCoinsEntry(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SendCoinsEntry),
    model(0)
{
    ui->setupUi(this);
