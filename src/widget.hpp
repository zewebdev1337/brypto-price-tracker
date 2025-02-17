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
#include <QGraphicsOpacityEffect>
#include <QSettings>
#include <QTimer>
#include <QPainter>

class PriceWidget : public QWidget {
    Q_OBJECT

public:
    explicit PriceWidget(const std::vector<QString>& symbols, QWidget* parent = nullptr);
    ~PriceWidget() override = default;

    static std::vector<QString> loadSymbolsFromConfig();
    static const std::vector<QString> defaultSymbols;

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override; 
private slots:
    void updatePrices();
    void handleNetworkReply(QNetworkReply* reply);

private:
    static constexpr float HOVER_OPACITY = 0.85;
    static constexpr float ACTIVE_OPACITY = 1.0;
    static constexpr float INACTIVE_OPACITY = 0.75;
    float currentOpacity{INACTIVE_OPACITY};
    float targetOpacity{INACTIVE_OPACITY};
    QTimer fadeTimer;
    void initUI();
    void updateLabel(const QString& symbol, const QString& price);
    static void saveSymbolsToConfig(const std::vector<QString>& symbolsToSave);
    void updateOpacity(float target);

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
    QWidget* contentWidget{nullptr};
    QGraphicsOpacityEffect* opacityEffect{nullptr};
};
