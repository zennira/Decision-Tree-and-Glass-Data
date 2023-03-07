#include "rpcconsole.h"
#include "ui_rpcconsole.h"

#include "clientmodel.h"
#include "altcoinrpc.h"
#include "guiutil.h"

#include <QTime>
#include <QTimer>
#include <QThread>
#include <QTextEdit>
#include <QKeyEvent>
#include <QUrl>
#include <QScrollBar>

#include <boost/tokenizer.hpp>
#include <openssl/crypto.h>

// TODO: make it possible to filter out categories (esp debug messages when implemented)
// TODO: receive errors and debug messages through ClientModel

const int CONSOLE_SCROLLBACK = 50;
const int CONSOLE_HISTORY = 50;

const QSize ICON_SIZE(24, 24);

const struct {
    const char *url;
    const char *source;
} ICON_MAPPING[] = {
    {"cmd-request", ":/icons/tx_input"},
    {"cmd-reply", ":/icons/tx_output"},
    {"cmd-error", ":/icons/tx_output"},
    {"misc", ":/icons/tx_inout"},
    {NULL, NULL}
};

/* Object for executing console RPC commands in a separate thread.
*/
class RPCExecutor: public QObject
{
    Q_OBJECT
public slots:
    void start();
    void request(const QString &command);
signals:
    void reply(int category, const QString &command);
};

#include "rpcconsole.moc"

void RPCExecutor::start()
{
   // Nothing to do
}

void RPCExecutor::request(const QString &command)
{
    // Parse shell-like command line into separate arguments
    std::string strMethod;
    std::vector<std::string> strParams;
    try {
        boost::escaped_list_separator<char> els('\\',' ','\"');
        std::string strCommand = command.toStdString();
        boost::tokenizer<boost::escaped_list_separator<char> > tok(strCommand, els);

        int n = 0;
        for(boost::tokenizer<boost::escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg,++n)
        {
            if(n == 0) // First parameter is the command
                strMethod = *beg;
            else
                strParams.push_back(*beg);
        }
    }
    catch(boost::escaped_list_error &e)
    {
        emit reply(RPCConsole::CMD_ERROR, QString("Parse error"));
        return;
    }

    try {
        std::string strPrint;
        json_spirit::Value result = tableRPC.execute(strMethod, RPCConvertValues(strMethod, strParams));

        // Format result reply
        if (result.type() == json_spirit::null_type)
            strPrint = "";
        else if (result.type() == json_spirit::str_type)
            strPrint = result.get_str();
        else
            strPrint = write_string(result, true);

        emit reply(RPCConsole::CMD_REPLY, QString::fromStdString(strPrint));
    }
    catch (json_spirit::Object& objError)
    {
        emit reply(RPCConsole::CMD_ERROR, QString::fromStdString(write_string(json_spirit::Value(objError), false)));
    }
    catch (std::exception& e)
    {
        emit reply(RPCConsole::CMD_ERROR, QString("Error: ") + QString::fromStdString(e.what()));
    }
}

RPCConsole::RPCConsole(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RPCConsole),
    historyPtr(0)
{
    ui->setupUi(this);

#ifndef Q_WS_MAC
    ui->openDebugLogfileButton->setIcon(QIcon(":/icons