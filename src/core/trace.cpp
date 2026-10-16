#include "core/trace.hpp"

#include <algorithm>
#include <format>

namespace gav {

namespace {

// 佇列太長時畫面放不下，只留前面幾項
constexpr std::size_t MaxContainerItems = 14;

template <typename T>
void eraseId(std::vector<std::pair<int, T>>& list, int id) {
    std::erase_if(list, [id](const auto& p) { return p.first == id; });
}

template <typename T>
bool containsId(const std::vector<std::pair<int, T>>& list, int id) {
    return std::any_of(list.begin(), list.end(), [id](const auto& p) { return p.first == id; });
}

}  // namespace

void applyStep(VisualState& s, const Step& step) {
    for (const auto& [u, st] : step.nodes) s.nodes[u] = st;
    for (const auto& [u, text] : step.nodeLabels) s.nodeLabels[u] = text;
    for (const auto& [e, st] : step.edges) s.edges[e] = st;
    for (const auto& [i, text] : step.cells) s.matrix[i] = text;
    for (const auto& [i, m] : step.marks) s.marks[i] = m;
    for (const auto& [i, v] : step.values) s.array[i] = v;
    for (const auto& [i, b] : step.bars) s.bars[i] = b;
    if (step.pointers) s.pointers = *step.pointers;
    if (step.container) s.container = *step.container;
    s.text = step.text;
    s.line = step.line;
}

void Player::load(const Trace* trace) {
    trace_ = trace;
    state_ = trace->initial;
    index_ = 0;
}

int Player::count() const {
    return trace_ ? static_cast<int>(trace_->steps.size()) : 0;
}

void Player::seek(int index) {
    if (!trace_) return;
    index = std::clamp(index, 0, count());
    if (index < index_) {
        state_ = trace_->initial;
        index_ = 0;
    }
    while (index_ < index) applyStep(state_, trace_->steps[index_++]);
}

Recorder::Recorder(const Graph& g, std::string algorithm, std::vector<std::string> pseudocode,
                   std::string containerTitle) {
    trace_.algorithm = std::move(algorithm);
    trace_.pseudocode = std::move(pseudocode);
    trace_.containerTitle = std::move(containerTitle);
    cur_.nodes.assign(g.size(), NodeState::Unvisited);
    cur_.nodeLabels.assign(g.size(), "");
    cur_.edges.assign(g.edges.size(), EdgeState::Normal);
    cur_.text = "按「播放」或空白鍵開始。";
    trace_.initial = cur_;
}

void Recorder::setNode(int u, NodeState s) {
    if (cur_.nodes[u] == s) return;
    cur_.nodes[u] = s;
    pending_.nodes.emplace_back(u, s);
}

void Recorder::setEdge(int e, EdgeState s) {
    if (cur_.edges[e] == s) return;
    cur_.edges[e] = s;
    pending_.edges.emplace_back(e, s);
}

void Recorder::setMark(int idx, CellMark m) {
    if (cur_.marks[idx] == m) return;
    cur_.marks[idx] = m;
    pending_.marks.emplace_back(idx, m);
}

void Recorder::node(int u, NodeState s) {
    eraseId(nodeFlash_, u);  // 明確設定的狀態優先於 flash
    setNode(u, s);
}

void Recorder::flashNode(int u, NodeState s) {
    if (!containsId(nodeFlash_, u)) nodeFlash_.emplace_back(u, cur_.nodes[u]);
    setNode(u, s);
}

void Recorder::label(int u, std::string text) {
    if (cur_.nodeLabels[u] == text) return;
    cur_.nodeLabels[u] = text;
    pending_.nodeLabels.emplace_back(u, std::move(text));
}

void Recorder::edge(int e, EdgeState s) {
    eraseId(edgeFlash_, e);
    setEdge(e, s);
}

void Recorder::flashEdge(int e, EdgeState s) {
    if (!containsId(edgeFlash_, e)) edgeFlash_.emplace_back(e, cur_.edges[e]);
    setEdge(e, s);
}

void Recorder::initMatrix(int n) {
    cur_.matrixSize = n;
    cur_.matrix.assign(static_cast<std::size_t>(n) * n, "");
    cur_.marks.assign(static_cast<std::size_t>(n) * n, CellMark::None);
    trace_.initial.matrixSize = n;
    trace_.initial.matrix = cur_.matrix;
    trace_.initial.marks = cur_.marks;
}

void Recorder::cell(int i, int j, std::string text) {
    const int idx = i * cur_.matrixSize + j;
    if (cur_.matrix[idx] == text) return;
    cur_.matrix[idx] = text;
    pending_.cells.emplace_back(idx, std::move(text));
}

void Recorder::flashMark(int i, int j, CellMark m) {
    const int idx = i * cur_.matrixSize + j;
    if (!containsId(markFlash_, idx)) markFlash_.emplace_back(idx, cur_.marks[idx]);
    setMark(idx, m);
}

void Recorder::initArray(std::vector<int> values) {
    cur_.bars.assign(values.size(), BarState::Normal);
    cur_.array = std::move(values);
    trace_.initial.array = cur_.array;
    trace_.initial.bars = cur_.bars;
}

void Recorder::value(int i, int v) {
    if (cur_.array[i] == v) return;
    cur_.array[i] = v;
    pending_.values.emplace_back(i, v);
}

void Recorder::setBar(int i, BarState s) {
    if (cur_.bars[i] == s) return;
    cur_.bars[i] = s;
    pending_.bars.emplace_back(i, s);
}

void Recorder::bar(int i, BarState s) {
    eraseId(barFlash_, i);
    setBar(i, s);
}

void Recorder::flashBar(int i, BarState s) {
    if (!containsId(barFlash_, i)) barFlash_.emplace_back(i, cur_.bars[i]);
    setBar(i, s);
}

void Recorder::pointers(std::vector<Pointer> list) {
    if (list == cur_.pointers) return;
    cur_.pointers = list;
    pending_.pointers = std::move(list);
}

void Recorder::container(std::vector<std::string> items) {
    if (items.size() > MaxContainerItems) {
        const std::size_t hidden = items.size() - (MaxContainerItems - 1);
        items.resize(MaxContainerItems - 1);
        items.push_back(std::format("…還有 {} 項", hidden));
    }
    if (items == cur_.container) return;
    cur_.container = items;
    pending_.container = std::move(items);
}

void Recorder::step(int line, std::string text) {
    cur_.text = text;
    cur_.line = line;
    pending_.text = std::move(text);
    pending_.line = line;
    trace_.steps.push_back(std::move(pending_));
    pending_ = Step{};

    // flash 只維持一步：把原狀態寫進下一步的變化裡
    for (const auto& [u, s] : std::exchange(nodeFlash_, {})) setNode(u, s);
    for (const auto& [e, s] : std::exchange(edgeFlash_, {})) setEdge(e, s);
    for (const auto& [i, m] : std::exchange(markFlash_, {})) setMark(i, m);
    for (const auto& [i, b] : std::exchange(barFlash_, {})) setBar(i, b);
}

Trace Recorder::finish(Result result) {
    trace_.result = std::move(result);
    return std::move(trace_);
}

}  // namespace gav
