#include "skiplist/skiplist.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace tiny_lsm {

// ************************ SkipListIterator ************************
BaseIterator &SkipListIterator::operator++() {
  // TODO: Lab1.2 任务：实现SkipListIterator的++操作符
  // ? current 是当前节点指针, forward_[0] 是最底层链表的下一个节点
  return *this;
}

bool SkipListIterator::operator==(const BaseIterator &other) const {
  // TODO: Lab1.2 任务：实现SkipListIterator的==操作符
  // ? 需要先通过 get_type() 判断类型再做 dynamic_cast
  return false;
}

bool SkipListIterator::operator!=(const BaseIterator &other) const {
  // TODO: Lab1.2 任务：实现SkipListIterator的!=操作符
  return true;
}

SkipListIterator::value_type SkipListIterator::operator*() const {
  // TODO: Lab1.2 任务：实现SkipListIterator的*操作符
  // ? 若 current 为空需抛出异常
  return {"", ""};
}

IteratorType SkipListIterator::get_type() const {
  // TODO: Lab1.2 任务：实现SkipListIterator的get_type
  // ? 主要是为了熟悉基类的定义和继承关系, 返回 IteratorType::SkipListIterator
  return IteratorType::SkipListIterator; // placeholder, 请替换为正确实现
}

bool SkipListIterator::is_valid() const {
  return current && !current->key_.empty();
}
bool SkipListIterator::is_end() const { return current == nullptr; }

std::string SkipListIterator::get_key() const { return current->key_; }
std::string SkipListIterator::get_value() const { return current->value_; }
uint64_t SkipListIterator::get_tranc_id() const { return current->tranc_id_; }

// ************************ SkipList ************************
// 构造函数
SkipList::SkipList(int max_lvl) : max_level(max_lvl), current_level(1) {
  head = std::make_shared<SkipListNode>("", "", max_level, 0);
  dis_01 = std::uniform_int_distribution<>(0, 1);
  dis_level = std::uniform_int_distribution<>(0, (1 << max_lvl) - 1);
  gen = std::mt19937(std::random_device()());
}

int SkipList::random_level() {
  // ? 通过"抛硬币"的方式随机生成层数：
  // ? - 每次有50%的概率增加一层
  // ? - 确保层数分布为：第1层100%，第2层50%，第3层25%，以此类推
  // ? - 层数范围限制在[1, max_level]之间，避免浪费内存
  // TODO: Lab1.1 任务：插入时随机为这一次操作确定其最高连接的链表层数
  int level = 1;
  while(dis_01(gen) ==1 && level < max_level){
    level++;
  }
  return level;

  return 0;
}

// 插入或更新键值对
void SkipList::put(const std::string &key, const std::string &value,
                   uint64_t tranc_id){
  spdlog::trace("SkipList--put({}, {}, {})", key, value, tranc_id);

  // TODO: Lab1.1 任务：实现插入或更新键值对
  // ? Hint: 你需要保证不同`Level`的步长从底层到高层逐渐增加
  // ? 你可能需要使用到`random_level`函数以确定层数, 其注释中为你提供一种思路
  // ? tranc_id 为事务id, 直接将其传递到 SkipListNode 的构造函数中即可
  // ? 若key存在且tranc_id相同, 仅更新value; 否则插入新节点
  // ? 注意维护 size_bytes
  // 1. 获取随机层级，如果超过了当前的最高层级，则更新当前的最高层级
  int new_level = random_level();
  if(new_level > current_level){
    current_level = new_level;
  }
    // 2. 找到要插入的位置
  // update数组用来存储每一层新节点应该插在哪个节点后面（即前驱节点）
  std::vector<std::shared_ptr<SkipListNode>> update(current_level, head);
  auto current= head;

  for(int level = current_level - 1; level >= 0; level--){
    while(current->forward_[level] != nullptr && current->forward_[level]->key_<key){
      current = current ->forward_[level];
    }
    update[level] =current;
  }
  // 3. 检查底层的下一个节点，看是不是已经存在相同的 key
  auto next_node = update[0]->forward_[0];
  if(next_node != nullptr && next_node->key_ ==key){
     // key 相同，还需要检查 tranc_id（事务ID）
     if(next_node->tranc_id_ == tranc_id){
      //减去旧 value 的长度，加上新 value 的长度
      size_bytes = size_bytes - next_node->value_.length();
      next_node->value_ = value;
      return;
     }
         // 如果 tranc_id 不同，说明这是一个新版本，应该作为新节点插入（多版本并发控制）
  }
    auto new_node = std::make_shared<SkipListNode>(key, value, new_level, tranc_id);
    // 5. 调整指针，将新节点插入到它需要存在的各个层级中
  for (int level = 0; level < new_level; level++) {
    // 新节点的下一个节点 = 前驱节点的下一个节点
    new_node->forward_[level] = update[level]->forward_[level];
    
    // 前驱节点的下一个节点 = 新节点
    update[level]->forward_[level] = new_node;
    
    // 跳表还需要维护后向指针 backward_
    new_node->set_backward(level, update[level]);
    
    if (new_node->forward_[level] != nullptr) {
      new_node->forward_[level]->set_backward(level, new_node);
    }
  }

  // 6. 维护整体内存大小：增加新节点的大小
  size_bytes += key.length() + value.length() + sizeof(uint64_t);
}

// 查找键值对
SkipListIterator SkipList::get(const std::string &key, uint64_t tranc_id) {
  spdlog::trace("SkipList--get({}) called", key);
  // TODO: Lab1.1 任务：实现查找键值对
  // ? 从最高层开始向下查找, 最终在底层确认 key 是否存在
  // ? 若 tranc_id == 0, 直接比较 key 返回; 否则需满足事务可见性 (tranc_id_ <= tranc_id)
  // TODO: 完成查找后还需要额外实现SkipListIterator中的TODO部分(Lab1.2)
  std::vector<std::shared_ptr<SkipListNode>> update(current_level,head);
  auto current = head;
  //1.查找
  for(int level = current-1; level >= 0; level--){
    while(current ->forward_[level] !=nullptr && current ->forward_[level]->key_<key){
      update[level] = current;
    }
  }
  //2,检查
  auto target = current->forward_[0];
  //如果不存在,暂时还不会
  //存在
  if(tranc_id == 0 || target->tranc_id_ <= tranc_id){
    return SkipListIterator(target);
  }

  return SkipListIterator{};
}

// 删除键值对
// ! 这里的 remove 是跳表本身真实的 remove,  lsm 应该使用 put 空值表示删除,
// ! 这里只是为了实现完整的 SkipList 不会真正被上层调用1
void SkipList::remove(const std::string &key) {
  // TODO: Lab1.1 任务：实现删除键值对
  // ? 从最高层开始查找目标节点并更新各层指针
  // ? 注意同时维护 backward_ 指针和 size_bytes

spdlog::trace("SkipList--remove({}) called", key);//写日记
// 1. 从最高层开始，找到每一层中目标节点的前驱节点
std::vector<std::shared_ptr<SkipListNode>> update(current_level,head);
auto current = head;
 
//从顶层到底层查找前驱
for(int level = current_level - 1;level >= 0; level--){
  while(current -> forward_[level] != nullptr && current ->forward_[level]->key_<key){
    current = current ->forward_[level];
  }
  update[level] = current;
}
//检查底层的下一节点是不是要删除的
auto target = update[0]->forward_[0];
if (target == nullptr|| target->key_!=key) {
  return;//没找到,直接返回
}
//3更新各层级指针,删除节点
for (int level = 0; level < target->forward_.size();level++){
  //前驱节点的 forward_ 指向目标节点的后继
  update[level] ->forward_[level] = target->forward_[level];
// 如果目标节点的后继不为空，更新其 backward_ 指针
  if(target->forward_[level] != nullptr){
  target->forward_[level]->set_backward(level, update[level]);
  }
}
// 4. 更新内存大小：减少删除节点占用的内存
 size_bytes -= target->key_.length() + target->value_.length() + sizeof(uint64_t);
}

// 刷盘时可以直接遍历最底层链表
std::vector<std::tuple<std::string, std::string, uint64_t>> SkipList::flush() {
  // std::shared_lock<std::shared_mutex> slock(rw_mutex);
  spdlog::debug("SkipList--flush(): Starting to flush skiplist data");

  std::vector<std::tuple<std::string, std::string, uint64_t>> data;
  auto node = head->forward_[0];
  while (node) {
    data.emplace_back(node->key_, node->value_, node->tranc_id_);
    node = node->forward_[0];
  }

  spdlog::debug("SkipList--flush(): Flushed {} entries", data.size());

  return data;
}

size_t SkipList::get_size() {
  // std::shared_lock<std::shared_mutex> slock(rw_mutex);
  return size_bytes;
}

// 清空跳表，释放内存
void SkipList::clear() {
  // std::unique_lock<std::shared_mutex> lock(rw_mutex);
  head = std::make_shared<SkipListNode>("", "", max_level, 0);
  size_bytes = 0;
}

SkipListIterator SkipList::begin() {
  // return SkipListIterator(head->forward[0], rw_mutex);
  return SkipListIterator(head->forward_[0]);
}

SkipListIterator SkipList::end() {
  return SkipListIterator(); // 使用空构造函数
}

// 找到前缀的起始位置
// 返回第一个前缀匹配或者大于前缀的迭代器
SkipListIterator SkipList::begin_preffix(const std::string &preffix) {
  // TODO: Lab1.3 任务：实现前缀查询的起始位置
  // ? 从最高层开始查找, 找到第一个 key >= preffix 的节点
  return SkipListIterator{};
}

// 找到前缀的终结位置
SkipListIterator SkipList::end_preffix(const std::string &prefix) {
  // TODO: Lab1.3 任务：实现前缀查询的终结位置
  // ? 找到第一个 key 不以 prefix 开头的节点作为终结位置
  return SkipListIterator{};
}

// ? 这里单调谓词的含义是, 整个数据库只会有一段连续区间满足此谓词
// ? 例如之前特化的前缀查询，以及后续可能的范围查询，都可以转化为谓词查询
// ? 返回第一个满足谓词的位置和最后一个满足谓词的迭代器
// ? 如果不存在, 返回 nullopt
// ? 谓词作用于key, 且保证满足谓词的结果只在一段连续的区间内, 例如前缀匹配的谓词
// ? predicate返回值:
// ?   0: 满足谓词
// ?   >0: 不满足谓词, 需要向右移动
// ?   <0: 不满足谓词, 需要向左移动
// ! Skiplist 中的谓词查询不会进行事务id的判断, 需要上层自己进行判断
std::optional<std::pair<SkipListIterator, SkipListIterator>>
SkipList::iters_monotony_predicate(
    std::function<int(const std::string &)> predicate) {
  // TODO: Lab1.3 任务：实现谓词查询
  // ? 分两步: 1. 利用多层跳表快速找到谓词满足区间内的一个节点
  // ?         2. 分别向前/向后扩展, 利用 backward_ 和 forward_ 确定区间边界
  // ? 注意: 向前查找时需要利用 backward_ 指针从当前节点的最高层开始回溯
  return std::nullopt;
}

// ? 打印跳表, 你可以在出错时调用此函数进行调试
void SkipList::print_skiplist() {
  for (int level = 0; level < current_level; level++) {
    std::cout << "Level " << level << ": ";
    auto current = head->forward_[level];
    while (current) {
      std::cout << current->key_;
      current = current->forward_[level];
      if (current) {
        std::cout << " -> ";
      }
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
} // namespace tiny_lsm
