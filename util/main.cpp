#include <QString>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QTextStream out(stdout);

    QString input(argv[1]);
    out << "const char array[] = {";

    for (int i = 0; i < input.length() / 2; i++) {
        out << "0x" << input.mid(i*2,2);
        if ((i+1) < input.length() / 2) {
            out << ",";
        }
    }

    out << "};" << endl;
    return 0;
}
