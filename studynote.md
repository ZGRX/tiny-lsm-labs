day1(4.1)
std::vector — 动态数组
std::shared_ptr / std::weak_ptr — 智能指针
std::map / std::set — 键值对/集合容器
std::string — 字符串
std::tuple — 元组
迭代器 — 遍历容器的方式
可选/边学边用的：

std::function — 函数对象
std::optional — 可选值
std::uniform_int_distribution — 随机数（这个项目用到但不是核心）
学习策略：

先学本质 — 理解跳表、内存表、SST等数据结构的逻辑
再学用法 — 当遇到某个标准库类，查文档学怎么用
不要死记 — 标准库文档随时可查，关键是理解思想

void SkipList::put(const std::string &key, const std::string &value,
                   uint64_t tranc_id){
  spdlog::trace("SkipList--put({}, {}, {})", key, value, tranc_id);

  // 1. 获取随机层级，如果超过了当前的最高层级，则更新当前的最高层级
  int new_level = random_level();
  if (new_level > current_level) {
    current_level = new_level;
  }

  // 2. 找到要插入的位置
  // update数组用来存储每一层新节点应该插在哪个节点后面（即前驱节点）
  std::vector<std::shared_ptr<SkipListNode>> update(current_level);
  auto current = head;

  // 从顶层到底层，寻找前驱节点
  for (int level = current_level - 1; level >= 0; level--) {
    while (current->forward_[level] != nullptr && 
           current->forward_[level]->key_ < key) {
      current = current->forward_[level];
    }
    update[level] = current;
  }

  // 3. 检查底层的下一个节点，看是不是已经存在相同的 key
  auto next_node = update[0]->forward_[0];
  if (next_node != nullptr && next_node->key_ == key) {
    // key 相同，还需要检查 tranc_id（事务ID）
    if (next_node->tranc_id_ == tranc_id) {
      // tranc_id 也相同，说明是直接更新 value 即可
      // 减去旧 value 的长度，加上新 value 的长度
      size_bytes = size_bytes - next_node->value_.length() + value.length();
      next_node->value_ = value;
      return; 
    }
    // 如果 tranc_id 不同，说明这是一个新版本，应该作为新节点插入（多版本并发控制）
  }

  // 4. 创建新节点
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