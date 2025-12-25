#include "modelparameter.h"
#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QDebug>

// 单例指针初始化
ModelParameter* ModelParameter::m_instance = nullptr;

// 构造函数：初始化默认参数
ModelParameter::ModelParameter(QObject* parent) : QObject(parent), m_hasLoaded(false)
{
    // 初始化默认物理参数值
    m_phi = 0.05;
    m_h = 20.0;
    m_mu = 0.5;
    m_B = 1.05;
    m_Ct = 5e-4;
    m_q = 50.0;
    m_rw = 0.1;
    m_projectPath = "";
    m_projectFilePath = "";
}

// 获取单例实例
ModelParameter* ModelParameter::instance()
{
    if (!m_instance) {
        m_instance = new ModelParameter();
    }
    return m_instance;
}

// 设置参数（用于新建项目时的初始化）
void ModelParameter::setParameters(double phi, double h, double mu, double B, double Ct, double q, double rw, const QString& path)
{
    // 更新内存变量
    m_phi = phi;
    m_h = h;
    m_mu = mu;
    m_B = B;
    m_Ct = Ct;
    m_q = q;
    m_rw = rw;

    m_projectFilePath = path; // 保存完整文件路径

    // 提取并保存目录路径
    QFileInfo fi(path);
    if (fi.isFile()) {
        m_projectPath = fi.absolutePath();
    } else {
        m_projectPath = path;
    }

    m_hasLoaded = true;

    // 如果是新建项目，m_fullProjectData 可能为空，需要初始化基本的 JSON 结构
    // 这样后续 saveProject 时才有完整结构
    if (m_fullProjectData.isEmpty()) {
        QJsonObject reservoir;
        reservoir["porosity"] = m_phi;
        reservoir["thickness"] = m_h;
        reservoir["wellRadius"] = m_rw;
        reservoir["productionRate"] = m_q;

        QJsonObject pvt;
        pvt["viscosity"] = m_mu;
        pvt["volumeFactor"] = m_B;
        pvt["compressibility"] = m_Ct;

        m_fullProjectData["reservoir"] = reservoir;
        m_fullProjectData["pvt"] = pvt;
    }
}

// 加载项目文件
bool ModelParameter::loadProject(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开项目文件:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "项目文件格式错误";
        return false;
    }

    m_fullProjectData = doc.object(); // 缓存整个JSON对象

    // 解析 reservoir (储层) 部分
    QJsonObject reservoir = m_fullProjectData["reservoir"].toObject();
    m_q = reservoir["productionRate"].toDouble(50.0);
    m_phi = reservoir["porosity"].toDouble(0.05);
    m_h = reservoir["thickness"].toDouble(20.0);
    m_rw = reservoir["wellRadius"].toDouble(0.1);

    // 解析 pvt (流体) 部分
    QJsonObject pvt = m_fullProjectData["pvt"].toObject();
    m_Ct = pvt["compressibility"].toDouble(5e-4);
    m_mu = pvt["viscosity"].toDouble(0.5);
    m_B = pvt["volumeFactor"].toDouble(1.05);

    // 保存项目路径信息
    m_projectFilePath = filePath;
    QFileInfo fi(filePath);
    m_projectPath = fi.absolutePath();

    m_hasLoaded = true;
    qDebug() << "项目参数加载成功, 路径:" << m_projectPath;
    return true;
}

// [新增] 保存当前项目
bool ModelParameter::saveProject()
{
    // 如果没有加载项目或路径为空，无法保存
    if (!m_hasLoaded || m_projectFilePath.isEmpty()) {
        qDebug() << "保存失败：没有打开的项目或路径无效";
        return false;
    }

    // 1. 将当前内存中的最新参数更新到 m_fullProjectData JSON 对象中
    // 确保 reservoir 节点存在或更新
    QJsonObject reservoir;
    if(m_fullProjectData.contains("reservoir")) reservoir = m_fullProjectData["reservoir"].toObject();
    reservoir["porosity"] = m_phi;
    reservoir["thickness"] = m_h;
    reservoir["wellRadius"] = m_rw;
    reservoir["productionRate"] = m_q;
    m_fullProjectData["reservoir"] = reservoir;

    // 确保 pvt 节点存在或更新
    QJsonObject pvt;
    if(m_fullProjectData.contains("pvt")) pvt = m_fullProjectData["pvt"].toObject();
    pvt["viscosity"] = m_mu;
    pvt["volumeFactor"] = m_B;
    pvt["compressibility"] = m_Ct;
    m_fullProjectData["pvt"] = pvt;

    // 注意：fitting (拟合结果) 已经在 saveFittingResult 中被更新到 m_fullProjectData 了
    // 这里不需要额外处理，直接写入即可

    // 2. 写入文件
    QFile file(m_projectFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法打开文件进行写入:" << m_projectFilePath;
        return false;
    }

    QJsonDocument doc(m_fullProjectData);
    file.write(doc.toJson()); // 写入 JSON 文本
    file.close();

    qDebug() << "项目已成功保存至:" << m_projectFilePath;
    return true;
}

// [新增] 关闭项目
void ModelParameter::closeProject()
{
    // 1. 重置状态标志
    m_hasLoaded = false;

    // 2. 清空路径信息
    m_projectPath.clear();
    m_projectFilePath.clear();

    // 3. 清空数据缓存
    m_fullProjectData = QJsonObject();

    // 4. 重置参数为默认值 (防止下次新建前残留旧数据)
    m_phi = 0.05;
    m_h = 20.0;
    m_mu = 0.5;
    m_B = 1.05;
    m_Ct = 5e-4;
    m_q = 50.0;
    m_rw = 0.1;

    qDebug() << "项目已关闭，内存已重置";
}

// 保存拟合结果（更新到内存缓存，并尝试写入文件）
void ModelParameter::saveFittingResult(const QJsonObject& fittingData)
{
    if (m_projectFilePath.isEmpty()) return;

    // 更新内存中的 fitting 字段
    m_fullProjectData["fitting"] = fittingData;

    // 立即写回文件，保证数据安全
    QFile file(m_projectFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_fullProjectData);
        file.write(doc.toJson());
        file.close();
        qDebug() << "拟合结果已保存到:" << m_projectFilePath;
    } else {
        qDebug() << "保存拟合结果失败，无法写入文件";
    }
}

// 获取拟合结果
QJsonObject ModelParameter::getFittingResult() const
{
    if (m_fullProjectData.contains("fitting")) {
        return m_fullProjectData["fitting"].toObject();
    }
    return QJsonObject();
}
