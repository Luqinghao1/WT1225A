#ifndef MODELPARAMETER_H
#define MODELPARAMETER_H

#include <QString>
#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>

// 项目参数单例类
// 用于在不同模块间共享项目基础信息，并负责项目文件的读取与写入
class ModelParameter : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例
    static ModelParameter* instance();

    // ========================================================================
    // 项目文件管理接口
    // ========================================================================

    // 加载项目文件 (.wtproj / .pwt)
    // 参数 filePath: 文件完整路径
    // 返回: 成功返回 true，失败返回 false
    bool loadProject(const QString& filePath);

    // [新增] 保存当前项目
    // 将内存中的参数写入当前关联的文件中
    // 返回: 成功返回 true
    bool saveProject();

    // [新增] 关闭当前项目
    // 重置内存中的所有参数为默认值，清空文件路径关联
    void closeProject();

    // 判断当前是否已加载了有效的项目
    bool hasLoadedProject() const { return m_hasLoaded; }

    // 获取当前项目所在的目录路径
    QString getProjectPath() const { return m_projectPath; }

    // ========================================================================
    // 数据存取接口
    // ========================================================================

    // 设置基础参数（通常用于新建项目后立即更新内存数据）
    void setParameters(double phi, double h, double mu, double B, double Ct, double q, double rw, const QString& path);

    // 获取各项参数的接口
    double getPhi() const { return m_phi; } // 孔隙度
    double getH() const { return m_h; }     // 有效厚度
    double getMu() const { return m_mu; }   // 粘度
    double getB() const { return m_B; }     // 体积系数
    double getCt() const { return m_Ct; }   // 综合压缩系数
    double getQ() const { return m_q; }     // 产量
    double getRw() const { return m_rw; }   // 井筒半径

    // 保存拟合结果到内存缓存（随后通过 saveProject 写入磁盘）
    void saveFittingResult(const QJsonObject& fittingData);

    // 获取项目文件中存储的拟合结果
    QJsonObject getFittingResult() const;

private:
    // 私有构造函数，确保单例模式
    explicit ModelParameter(QObject* parent = nullptr);
    static ModelParameter* m_instance;

    // 项目状态标志
    bool m_hasLoaded;

    // 路径信息
    QString m_projectPath;      // 项目所在文件夹路径
    QString m_projectFilePath;  // 项目文件完整路径 (.wtproj)

    // 缓存完整的JSON对象，以便保存时不丢失其他未修改的信息
    QJsonObject m_fullProjectData;

    // 基础物理参数成员变量
    double m_phi; // 孔隙度
    double m_h;   // 厚度
    double m_mu;  // 粘度
    double m_B;   // 体积系数
    double m_Ct;  // 综合压缩系数
    double m_q;   // 产量
    double m_rw;  // 井筒半径
};

#endif // MODELPARAMETER_H
