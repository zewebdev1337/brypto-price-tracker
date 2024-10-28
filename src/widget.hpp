#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QPoint>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <memory>
#include <vector>
#include <shared_mutex>


class PriceWidget : public QWidget {
    Q_OBJECT

public:
    explicit PriceWidget(const std::vector<QString>& symbols, QWidget* parent = nullptr);
    ~PriceWidget() override = default;

    static std::vector<QString> loadSymbolsFromConfig();
    static const std::vector<QString> defaultSymbols;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void updatePrices();
    void handleNetworkReply(QNetworkReply* reply);

private:
    void initUI();
    void updateLabel(const QString& symbol, const QString& price);
    void loadConfig();
    void saveConfig() const;

    static void createDefaultConfig(const QString& path);

    const QString apiUrl{"https://api.binance.com/api/v3/ticker/price?symbol="};
    QMap<QString, QLabel*> priceLabels;
    std::vector<QString> symbols;
    QPoint oldPos;
    QVBoxLayout* layout{nullptr};
    
    std::unique_ptr<QNetworkAccessManager> networkManager;
    QTimer* updateTimer{nullptr};
    
    // Thread-safe price cache
    mutable std::shared_mutex priceMutex;
    QMap<QString, QString> priceCache;
};
