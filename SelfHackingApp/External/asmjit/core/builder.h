// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef _ASMJIT_CORE_BUILDER_H
#define _ASMJIT_CORE_BUILDER_H

#include "../core/build.h"
#ifndef ASMJIT_NO_BUILDER

#include "../core/assembler.h"
#include "../core/codeholder.h"
#include "../core/constpool.h"
#include "../core/inst.h"
#include "../core/operand.h"
#include "../core/string.h"
#include "../core/support.h"
#include "../core/zone.h"
#include "../core/zonevector.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_builder
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class BaseBuilder;
class Pass;

class BaseNode;
class InstNode;
class SectionNode;
class LabelNode;
class AlignNode;
class EmbedDataNode;
class EmbedLabelNode;
class ConstPoolNode;
class CommentNode;
class SentinelNode;
class LabelDeltaNode;

// ============================================================================
// [asmjit::BaseBuilder]
// ============================================================================

class ASMJIT_VIRTAPI BaseBuilder : public BaseEmitter {
public:
  ASMJIT_NONCOPYABLE(BaseBuilder)
  typedef BaseEmitter Base;

  //! Base zone used to allocate nodes and passes.
  Zone _codeZone;
  //! Data zone used to allocate data and names.
  Zone _dataZone;
  //! Pass zone, passed to `Pass::run()`.
  Zone _passZone;
  //! Allocator that uses `_codeZone`.
  ZoneAllocator _allocator;

  //! Array of `Pass` objects.
  ZoneVector<Pass*> _passes;
  //! Maps section indexes to `LabelNode` nodes.
  ZoneVector<SectionNode*> _sectionNodes;
  //! Maps label indexes to `LabelNode` nodes.
  ZoneVector<LabelNode*> _labelNodes;

  //! Current node (cursor).
  BaseNode* _cursor;
  //! First node of the current section.
  BaseNode* _firstNode;
  //! Last node of the current section.
  BaseNode* _lastNode;

  //! Flags assigned to each new node.
  uint32_t _nodeFlags;
  //! The sections links are dirty (used internally).
  bool _dirtySectionLinks;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `BaseBuilder` instance.
  ASMJIT_API BaseBuilder() noexcept;
  //! Destroys the `BaseBuilder` instance.
  ASMJIT_API virtual ~BaseBuilder() noexcept;

  //! \}

  //! \name Node Management
  //! \{

  //! Returns the first node.
  inline BaseNode* firstNode() const noexcept { return _firstNode; }
  //! Returns the last node.
  inline BaseNode* lastNode() const noexcept { return _lastNode; }

  //! Allocates and instantiates a new node of type `T` and returns its instance.
  //! If the allocation fails `nullptr` is returned.
  //!
  //! The template argument `T` must be a type that is extends \ref BaseNode.
  //!
  //! \remarks The pointer returned (if non-null) is owned by the Builder or
  //! Compiler. When the Builder/Compiler is destroyed it destroys all nodes
  //! it created so no manual memory management is required.
  template<typename T>
  inline T* newNodeT() noexcept {
    return _allocator.newT<T>(this);
  }

  //! \overload
  template<typename T, typename... ArgsT>
  inline T* newNodeT(ArgsT&&... args) noexcept {
    return _allocator.newT<T>(this, std::forward<ArgsT>(args)...);
  }

  //! Creates a new `LabelNode`.
  ASMJIT_API LabelNode* newLabelNode() noexcept;
  //! Creates a new `AlignNode`.
  ASMJIT_API AlignNode* newAlignNode(uint32_t alignMode, uint32_t alignment) noexcept;
  //! Creates a new `EmbedDataNode`.
  ASMJIT_API EmbedDataNode* newEmbedDataNode(const void* data, uint32_t size) noexcept;
  //! Creates a new `ConstPoolNode`.
  ASMJIT_API ConstPoolNode* newConstPoolNode() noexcept;
  //! Creates a new `CommentNode`.
  ASMJIT_API CommentNode* newCommentNode(const char* data, size_t size) noexcept;

  ASMJIT_API InstNode* newInstNode(uint32_t instId, uint32_t instOptions, const Operand_& o0) noexcept;
  ASMJIT_API InstNode* newInstNode(uint32_t instId, uint32_t instOptions, const Operand_& o0, const Operand_& o1) noexcept;
  ASMJIT_API InstNode* newInstNode(uint32_t instId, uint32_t instOptions, const Operand_& o0, const Operand_& o1, const Operand_& o2) noexcept;
  ASMJIT_API InstNode* newInstNode(uint32_t instId, uint32_t instOptions, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) noexcept;
  ASMJIT_API InstNode* newInstNodeRaw(uint32_t instId, uint32_t instOptions, uint32_t opCount) noexcept;

  //! Adds `node` after the current and sets the current node to the given `node`.
  ASMJIT_API BaseNode* addNode(BaseNode* node) noexcept;
  //! Inserts the given `node` after `ref`.
  ASMJIT_API BaseNode* addAfter(BaseNode* node, BaseNode* ref) noexcept;
  //! Inserts the given `node` before `ref`.
  ASMJIT_API BaseNode* addBefore(BaseNode* node, BaseNode* ref) noexcept;
  //! Removes the given `node`.
  ASMJIT_API BaseNode* removeNode(BaseNode* node) noexcept;
  //! Removes multiple nodes.
  ASMJIT_API void removeNodes(BaseNode* first, BaseNode* last) noexcept;

  //! Returns the cursor.
  //!
  //! When the Builder/Compiler is created it automatically creates a '.text'
  //! \ref SectionNode, which will be the initial one. When instructions are
  //! added they are always added after the cursor and the cursor is changed
  //! to be that newly added node. Use `setCursor()` to change where new nodes
  //! are inserted.
  inline BaseNode* cursor() const noexcept { return _cursor; }

  //! Sets the current node to `node` and return the previous one.
  ASMJIT_API BaseNode* setCursor(BaseNode* node) noexcept;

  //! Sets the current node without returning the previous node.
  //!
  //! Only use this function if you are concerned about performance and want
  //! this inlined (for example if you set the cursor in a loop, etc...).
  inline void _setCursor(BaseNode* node) noexcept { _cursor = node; }

  //! \}

  //! \name Section Management
  //! \{

  //! Returns a vector of SectionNode objects.
  //!
  //! \note If a section of some id is not associated with the Builder/Compiler
  //! it would be null, so always check for nulls if you iterate over the vector.
  inline const ZoneVector<SectionNode*>& sectionNodes() const noexcept { return _sectionNodes; }

  //! Tests whether the `SectionNode` of the given `sectionId` was registered.
  inline bool hasRegisteredSectionNode(uint32_t sectionId) const noexcept {
    return sectionId < _sectionNodes.size() && _sectionNodes[sectionId] != nullptr;
  }

  //! Returns or creates a `SectionNode` that matches the given `sectionId`.
  //!
  //! \remarks This function will either get the existing `SectionNode` or create
  //! it in case it wasn't created before. You can check whether a section has a
  //! registered `SectionNode` by using `BaseBuilder::hasRegisteredSectionNode()`.
  ASMJIT_API Error sectionNodeOf(SectionNode** pOut, uint32_t sectionId) noexcept;

  ASMJIT_API Error section(Section* section) override;

  //! Returns whether the section links of active section nodes are dirty. You can
  //! update these links by calling `updateSectionLinks()` in such case.
  inline bool hasDirtySectionLinks() const noexcept { return _dirtySectionLinks; }

  //! Updates links of all active section nodes.
  ASMJIT_API void updateSectionLinks() noexcept;

  //! \}

  //! \name Label Management
  //! \{

  //! Returns a vector of LabelNode nodes.
  //!
  //! \note If a label of some id is not associated with the Builder/Compiler
  //! it would be null, so always check for nulls if you iterate over the vector.
  inline const ZoneVector<LabelNode*>& labelNodes() const noexcept { return _labelNodes; }

  //! Tests whether the `LabelNode` of the given `labelId` was registered.
  inline bool hasRegisteredLabelNode(uint32_t labelId) const noexcept {
    return labelId < _labelNodes.size() && _labelNodes[labelId] != nullptr;
  }

  //! \overload
  inline bool hasRegisteredLabelNode(const Label& label) const noexcept {
    return hasRegisteredLabelNode(label.id());
  }

  //! Gets or creates a `LabelNode` that matches the given `labelId`.
  //!
  //! \remarks This function will either get the existing `LabelNode` or create
  //! it in case it wasn't created before. You can check whether a label has a
  //! registered `LabelNode` by using `BaseBuilder::hasRegisteredLabelNode()`.
  ASMJIT_API Error labelNodeOf(LabelNode** pOut, uint32_t labelId) noexcept;

  //! \overload
  inline Error labelNodeOf(LabelNode** pOut, const Label& label) noexcept {
    return labelNodeOf(pOut, label.id());
  }

  //! Registers this label node [Internal].
  //!
  //! This function is used internally to register a newly created `LabelNode`
  //! with this instance of Builder/Compiler. Use `labelNodeOf()` functions to
  //! get back `LabelNode` from a label or its identifier.
  ASMJIT_API Error registerLabelNode(LabelNode* node) noexcept;

  ASMJIT_API Label newLabel() override;
  ASMJIT_API Label newNamedLabel(const char* name, size_t nameSize = SIZE_MAX, uint32_t type = Label::kTypeGlobal, uint32_t parentId = Globals::kInvalidId) override;
  ASMJIT_API Error bind(const Label& label) override;

  //! \}

  //! \name Passes
  //! \{

  //! Returns a vector of `Pass` instances that will be executed by `runPasses()`.
  inline const ZoneVector<Pass*>& passes() const noexcept { return _passes; }

  //! Allocates and instantiates a new pass of type `T` and returns its instance.
  //! If the allocation fails `nullptr` is returned.
  //!
  //! The template argument `T` must be a type that is extends \ref Pass.
  //!
  //! \remarks The pointer returned (if non-null) is owned by the Builder or
  //! Compiler. When the Builder/Compiler is destroyed it destroys all passes
  //! it created so no manual memory management is required.
  template<typename T>
  inline T* newPassT() noexcept { return _codeZone.newT<T>(); }

  //! \overload
  template<typename T, typename... ArgsT>
  inline T* newPassT(ArgsT&&... args) noexcept { return _codeZone.newT<T>(std::forward<ArgsT>(args)...); }

  template<typename T>
  inline Error addPassT() noexcept { return addPass(newPassT<T>()); }

  template<typename T, typename... ArgsT>
  inline Error addPassT(ArgsT&&... args) noexcept { return addPass(newPassT<T, ArgsT...>(std::forward<ArgsT>(args)...)); }

  //! Returns `Pass` by name.
  ASMJIT_API Pass* passByName(const char* name) const noexcept;
  //! Adds `pass` to the list of passes.
  ASMJIT_API Error addPass(Pass* pass) noexcept;
  //! Removes `pass` from the list of passes and delete it.
  ASMJIT_API Error deletePass(Pass* pass) noexcept;

  //! Runs all passes in order.
  ASMJIT_API Error runPasses();

  //! \}

  //! \name Emit
  //! \{

  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) override;
  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, const Operand_& o5) override;

  //! \}

  //! \name Align
  //! \{

  ASMJIT_API Error align(uint32_t alignMode, uint32_t alignment) override;

  //! \}

  //! \name Embed
  //! \{

  ASMJIT_API Error embed(const void* data, uint32_t dataSize) override;
  ASMJIT_API Error embedLabel(const Label& label) override;
  ASMJIT_API Error embedLabelDelta(const Label& label, const Label& base, uint32_t dataSize) override;
  ASMJIT_API Error embedConstPool(const Label& label, const ConstPool& pool) override;

  //! \}

  //! \name Comment
  //! \{

  ASMJIT_API Error comment(const char* data, size_t size = SIZE_MAX) override;

  //! \}

  //! \name Serialization
  //! \{

  //! Serializes everything the given emitter `dst`.
  //!
  //! Although not explicitly required the emitter will most probably be of
  //! Assembler type. The reason is that there is no known use of serializing
  //! nodes held by Builder/Compiler into another Builder-like emitter.
  ASMJIT_API Error serialize(BaseEmitter* dst);

  //! \}

  //! \name Logging
  //! \{

  #ifndef ASMJIT_NO_LOGGING
  ASMJIT_API Error dump(String& sb, uint32_t flags = 0) const noexcept;
  #endif

  //! \}

  //! \name Events
  //! \{

  ASMJIT_API Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API Error onDetach(CodeHolder* code) noexcept override;

  //! \}
};

// ============================================================================
// [asmjit::BaseNode]
// ============================================================================

//! Base node.
//!
//! Every node represents a building-block used by `BaseBuilder`. It can be
//! instruction, data, label, comment, directive, or any other high-level
//! representation that can be transformed to the building blocks mentioned.
//! Every class that inherits `BaseBuilder` can define its own nodes that it
//! can lower to basic nodes.
class BaseNode {
public:
  ASMJIT_NONCOPYABLE(BaseNode)

  union {
    struct {
      //! Previous node.
      BaseNode* _prev;
      //! Next node.
      BaseNode* _next;
    };
    //! Links (previous and next nodes).
    BaseNode* _links[2];
  };

  //! Data shared between all types of nodes.
  struct AnyData {
    //! Node type, see \ref NodeType.
    uint8_t _nodeType;
    //! Node flags, see \ref Flags.
    uint8_t _nodeFlags;
    //! Not used by BaseNode.
    uint8_t _reserved0;
    //! Not used by BaseNode.
    uint8_t _reserved1;
  };

  struct InstData {
    //! Node type, see \ref NodeType.
    uint8_t _nodeType;
    //! Node flags, see \ref Flags.
    uint8_t _nodeFlags;
    //! Instruction operands count (used).
    uint8_t _opCount;
    //! Instruction operands capacity (allocated).
    uint8_t _opCapacity;
  };

  struct SentinelData {
    //! Node type, see \ref NodeType.
    uint8_t _nodeType;
    //! Node flags, see \ref Flags.
    uint8_t _nodeFlags;
    //! Sentinel type.
    uint8_t _sentinelType;
    //! Not used by BaseNode.
    uint8_t _reserved1;
  };

  union {
    AnyData _any;
    InstData _inst;
    SentinelData _sentinel;
  };

  //! Node position in code (should be unique).
  uint32_t _position;

  //! Value reserved for AsmJit users never touched by AsmJit itself.
  union {
    uint64_t _userDataU64;
    void* _userDataPtr;
  };

  //! Data used exclusively by the current `Pass`.
  void* _passData;

  //! Inline comment/annotation or nullptr if not used.
  const char* _inlineComment;

  //! Type of `BaseNode`.
  enum NodeType : uint32_t {
    //! Invalid node (internal, don't use).
    kNodeNone = 0,

    // [BaseBuilder]

    //! Node is `InstNode` or `InstExNode`.
    kNodeInst = 1,
    //! Node is `SectionNode`.
    kNodeSection = 2,
    //! Node is `LabelNode`.
    kNodeLabel = 3,
    //! Node is `AlignNode`.
    kNodeAlign = 4,
    //! Node is `EmbedDataNode`.
    kNodeEmbedData = 5,
    //! Node is `EmbedLabelNode`.
    kNodeEmbedLabel = 6,
    //! Node is `EmbedLabelDeltaNode`.
    kNodeEmbedLabelDelta = 7,
    //! Node is `ConstPoolNode`.
    kNodeConstPool = 8,
    //! Node is `CommentNode`.
    kNodeComment = 9,
    //! Node is `SentinelNode`.
    kNodeSentinel = 10,

    // [BaseCompiler]

    //! Node is `FuncNode` (acts as LabelNode).
    kNodeFunc = 16,
    //! Node is `FuncRetNode` (acts as InstNode).
    kNodeFuncRet = 17,
    //! Node is `FuncCallNode` (acts as InstNode).
    kNodeFuncCall = 18,

    // [UserDefined]

    //! First id of a user-defined node.
    kNodeUser = 32
  };

  //! Node flags, specify what the node is and/or does.
  enum Flags : uint32_t {
    kFlagIsCode          = 0x01u,        //!< Node is code that can be executed (instruction, label, align, etc...).
    kFlagIsData          = 0x02u,        //!< Node is data that cannot be executed (data, const-pool, etc...).
    kFlagIsInformative   = 0x04u,        //!< Node is informative, can be removed and ignored.
    kFlagIsRemovable     = 0x08u,        //!< Node can be safely removed if unreachable.
    kFlagHasNoEffect     = 0x10u,        //!< Node does nothing when executed (label, align, explicit nop).
    kFlagActsAsInst      = 0x20u,        //!< Node is an instruction or acts as it.
    kFlagActsAsLabel     = 0x40u,        //!< Node is a label or acts as it.
    kFlagIsActive        = 0x80u         //!< Node is active (part of the code).
  };

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `BaseNode` - always use `BaseBuilder` to allocate nodes.
  ASMJIT_INLINE BaseNode(BaseBuilder* cb, uint32_t type, uint32_t flags = 0) noexcept {
    _prev = nullptr;
    _next = nullptr;
    _any._nodeType = uint8_t(type);
    _any._nodeFlags = uint8_t(flags | cb->_nodeFlags);
    _any._reserved0 = 0;
    _any._reserved1 = 0;
    _position = 0;
    _userDataU64 = 0;
    _passData = nullptr;
    _inlineComment = nullptr;
  }

  //! \}

  //! \name Accessors
  //! \{

  //! Casts this node to `T*`.
  template<typename T>
  inline T* as() noexcept { return static_cast<T*>(this); }
  //! Casts this node to `const T*`.
  template<typename T>
  inline const T* as() const noexcept { return static_cast<const T*>(this); }

  //! Returns previous node or `nullptr` if this node is either first or not
  //! part of Builder/Compiler node-list.
  inline BaseNode* prev() const noexcept { return _prev; }
  //! Returns next node or `nullptr` if this node is either last or not part
  //! of Builder/Compiler node-list.
  inline BaseNode* next() const noexcept { return _next; }

  //! Returns the type of the node, see `NodeType`.
  inline uint32_t type() const noexcept { return _any._nodeType; }

  //! Sets the type of the node, see `NodeType` (internal).
  //!
  //! \remarks You should never set a type of a node to anything else than the
  //! initial value. This function is only provided for users that use custom
  //! nodes and need to change the type either during construction or later.
  inline void setType(uint32_t type) noexcept { _any._nodeType = uint8_t(type); }

  //! Tests whether this node is either `InstNode` or extends it.
  inline bool isInst() const noexcept { return hasFlag(kFlagActsAsInst); }
  //! Tests whether this node is `SectionNode`.
  inline bool isSection() const noexcept { return type() == kNodeSection; }
  //! Tests whether this node is either `LabelNode` or extends it.
  inline bool isLabel() const noexcept { return hasFlag(kFlagActsAsLabel); }
  //! Tests whether this node is `AlignNode`.
  inline bool isAlign() const noexcept { return type() == kNodeAlign; }
  //! Tests whether this node is `EmbedDataNode`.
  inline bool isEmbedData() const noexcept { return type() == kNodeEmbedData; }
  //! Tests whether this node is `EmbedLabelNode`.
  inline bool isEmbedLabel() const noexcept { return type() == kNodeEmbedLabel; }
  //! Tests whether this node is `EmbedLabelDeltaNode`.
  inline bool isEmbedLabelDelta() const noexcept { return type() == kNodeEmbedLabelDelta; }
  //! Tests whether this node is `ConstPoolNode`.
  inline bool isConstPool() const noexcept { return type() == kNodeConstPool; }
  //! Tests whether this node is `CommentNode`.
  inline bool isComment() const noexcept { return type() == kNodeComment; }
  //! Tests whether this node is `SentinelNode`.
  inline bool isSentinel() const noexcept { return type() == kNodeSentinel; }

  //! Tests whether this node is `FuncNode`.
  inline bool isFunc() const noexcept { return type() == kNodeFunc; }
  //! Tests whether this node is `FuncRetNode`.
  inline bool isFuncRet() const noexcept { return type() == kNodeFuncRet; }
  //! Tests whether this node is `FuncCallNode`.
  inline bool isFuncCall() const noexcept { return type() == kNodeFuncCall; }

  //! Returns the node flags, see \ref Flags.
  inline uint32_t flags() const noexcept { return _any._nodeFlags; }
  //! Tests whether the node has the given `flag` set.
  inline bool hasFlag(uint32_t flag) const noexcept { return (uint32_t(_any._nodeFlags) & flag) != 0; }
  //! Replaces node flags with `flags`.
  inline void setFlags(uint32_t flags) noexcept { _any._nodeFlags = uint8_t(flags); }
  //! Adds the given `flags` to node flags.
  inline void addFlags(uint32_t flags) noexcept { _any._nodeFlags = uint8_t(_any._nodeFlags | flags); }
  //! Clears the given `flags` from node flags.
  inline void clearFlags(uint32_t flags) noexcept { _any._nodeFlags = uint8_t(_any._nodeFlags & (flags ^ 0xFF)); }

  //! Tests whether the node is code that can be executed.
  inline bool isCode() const noexcept { return hasFlag(kFlagIsCode); }
  //! Tests whether the node is data that cannot be executed.
  inline bool isData() const noexcept { return hasFlag(kFlagIsData); }
  //! Tests whether the node is informative only (is never encoded like comment, etc...).
  inline bool isInformative() const noexcept { return hasFlag(kFlagIsInformative); }
  //! Tests whether the node is removable if it's in an unreachable code block.
  inline bool isRemovable() const noexcept { return hasFlag(kFlagIsRemovable); }
  //! Tests whether the node has no effect when executed (label, .align, nop, ...).
  inline bool hasNoEffect() const noexcept { return hasFlag(kFlagHasNoEffect); }
  //! Tests whether the node is part of the code.
  inline bool isActive() const noexcept { return hasFlag(kFlagIsActive); }

  //! Tests whether the node has a position assigned.
  //!
  //! \remarks Returns `true` if node position is non-zero.
  inline bool hasPosition() const noexcept { return _position != 0; }
  //! Returns node position.
  inline uint32_t position() const noexcept { return _position; }
  //! Sets node position.
  //!
  //! Node position is a 32-bit unsigned integer that is used by Compiler to
  //! track where the node is relatively to the start of the function. It doesn't
  //! describe a byte position in a binary, instead it's just a pseudo position
  //! used by liveness analysis and other tools around Compiler.
  //!
  //! If you don't use Compiler then you may use `position()` and `setPosition()`
  //! freely for your own purposes if the 32-bit value limit is okay for you.
  inline void setPosition(uint32_t position) noexcept { _position = position; }

  //! Returns user data casted to `T*`.
  //!
  //! User data is decicated to be used only by AsmJit users and not touched
  //! by the library. The data has a pointer size so you can either store a
  //! pointer or `intptr_t` value through `setUserDataAsIntPtr()`.
  template<typename T>
  inline T* userDataAsPtr() const noexcept { return static_cast<T*>(_userDataPtr); }
  //! Returns user data casted to `int64_t`.
  inline int64_t userDataAsInt64() const noexcept { return int64_t(_userDataU64); }
  //! Returns user data casted to `uint64_t`.
  inline uint64_t userDataAsUInt64() const noexcept { return _userDataU64; }

  //! Sets user data to `data`.
  template<typename T>
  inline void setUserDataAsPtr(T* data) noexcept { _userDataPtr = static_cast<void*>(data); }
  //! Sets used data to the given 64-bit signed `value`.
  inline void setUserDataAsInt64(int64_t value) noexcept { _userDataU64 = uint64_t(value); }
  //! Sets used data to the given 64-bit unsigned `value`.
  inline void setUserDataAsUInt64(uint64_t value) noexcept { _userDataU64 = value; }

  //! Resets user data to zero / nullptr.
  inline void resetUserData() noexcept { _userDataU64 = 0; }

  //! Tests whether the node has an associated pass data.
  inline bool hasPassData() const noexcept { return _passData != nullptr; }
  //! Returns the node pass data - data used during processing & transformations.
  template<typename T>
  inline T* passData() const noexcept { return (T*)_passData; }
  //! Sets the node pass data to `data`.
  template<typename T>
  inline void setPassData(T* data) noexcept { _passData = (void*)data; }
  //! Resets the node pass data to nullptr.
  inline void resetPassData() noexcept { _passData = nullptr; }

  //! Tests whether the node has an inline comment/annotation.
  inline bool hasInlineComment() const noexcept { return _inlineComment != nullptr; }
  //! Returns an inline comment/annotation string.
  inline const char* inlineComment() const noexcept { return _inlineComment; }
  //! Sets an inline comment/annotation string to `s`.
  inline void setInlineComment(const char* s) noexcept { _inlineComment = s; }
  //! Resets an inline comment/annotation string to nullptr.
  inline void resetInlineComment() noexcept { _inlineComment = nullptr; }

  //! \}
};

// ============================================================================
// [asmjit::InstNode]
// ============================================================================

//! Instruction node.
//!
//! Wraps an instruction with its options and operands.
class InstNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(InstNode)

  enum : uint32_t {
    //! Count of embedded operands per `InstNode` that are always allocated as
    //! a part of the instruction. Minimum embedded operands is 4, but in 32-bit
    //! more pointers are smaller and we can embed 5. The rest (up to 6 operands)
    //! is always stored in `InstExNode`.
    kBaseOpCapacity = uint32_t((128 - sizeof(BaseNode) - sizeof(BaseInst)) / sizeof(Operand_))
  };

  //! Base instruction data.
  BaseInst _baseInst;
  //! First 4 or 5 operands (indexed from 0).
  Operand_ _opArray[kBaseOpCapacity];

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `InstNode` instance.
  ASMJIT_INLINE InstNode(BaseBuilder* cb, uint32_t instId, uint32_t options, uint32_t opCount, uint32_t opCapacity = kBaseOpCapacity) noexcept
    : BaseNode(cb, kNodeInst, kFlagIsCode | kFlagIsRemovable | kFlagActsAsInst),
      _baseInst(instId, options) {
    _inst._opCapacity = uint8_t(opCapacity);
    _inst._opCount = uint8_t(opCount);
  }

  //! Reset all built-in operands, including `extraReg`.
  inline void _resetOps() noexcept {
    _baseInst.resetExtraReg();
    for (uint32_t i = 0, count = opCapacity(); i < count; i++)
      _opArray[i].reset();
  }

  //! \}

  //! \name Accessors
  //! \{

  inline BaseInst& baseInst() noexcept { return _baseInst; }
  inline const BaseInst& baseInst() const noexcept { return _baseInst; }

  //! Returns the instruction id, see `BaseInst::Id`.
  inline uint32_t id() const noexcept { return _baseInst.id(); }
  //! Sets the instruction id to `id`, see `BaseInst::Id`.
  inline void setId(uint32_t id) noexcept { _baseInst.setId(id); }

  //! Returns instruction options.
  inline uint32_t instOptions() const noexcept { return _baseInst.options(); }
  //! Sets instruction options.
  inline void setInstOptions(uint32_t options) noexcept { _baseInst.setOptions(options); }
  //! Adds instruction options.
  inline void addInstOptions(uint32_t options) noexcept { _baseInst.addOptions(options); }
  //! Clears instruction options.
  inline void clearInstOptions(uint32_t options) noexcept { _baseInst.clearOptions(options); }

  //! Tests whether the node has an extra register operand.
  inline bool hasExtraReg() const noexcept { return _baseInst.hasExtraReg(); }
  //! Returns extra register operand.
  inline RegOnly& extraReg() noexcept { return _baseInst.extraReg(); }
  //! \overload
  inline const RegOnly& extraReg() const noexcept { return _baseInst.extraReg(); }
  //! Sets extra register operand to `reg`.
  inline void setExtraReg(const BaseReg& reg) noexcept { _baseInst.setExtraReg(reg); }
  //! Sets extra register operand to `reg`.
  inline void setExtraReg(const RegOnly& reg) noexcept { _baseInst.setExtraReg(reg); }
  //! Resets extra register operand.
  inline void resetExtraReg() noexcept { _baseInst.resetExtraReg(); }

  //! Returns operands count.
  inline uint32_t opCount() const noexcept { return _inst._opCount; }
  //! Returns operands capacity.
  inline uint32_t opCapacity() const noexcept { return _inst._opCapacity; }

  //! Sets operands count.
  inline void setOpCount(uint32_t opCount) noexcept { _inst._opCount = uint8_t(opCount); }

  //! Returns operands array.
  inline Operand* operands() noexcept { return (Operand*)_opArray; }
  //! Returns operands array (const).
  inline const Operand* operands() const noexcept { return (const Operand*)_opArray; }

  inline Operand& opType(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < opCapacity());
    return _opArray[index].as<Operand>();
  }

  inline const Operand& opType(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < opCapacity());
    return _opArray[index].as<Operand>();
  }

  inline void setOp(uint32_t index, const Operand_& op) noexcept {
    ASMJIT_ASSERT(index < opCapacity());
    _opArray[index].copyFrom(op);
  }

  inline void resetOp(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < opCapacity());
    _opArray[index].reset();
  }

  //! \}

  //! \name Utilities
  //! \{

  inline bool hasOpType(uint32_t opType) const noexcept {
    for (uint32_t i = 0, count = opCount(); i < count; i++)
      if (_opArray[i].opType() == opType)
        return true;
    return false;
  }

  inline bool hasRegOp() const noexcept { return hasOpType(Operand::kOpReg); }
  inline bool hasMemOp() const noexcept { return hasOpType(Operand::kOpMem); }
  inline bool hasImmOp() const noexcept { return hasOpType(Operand::kOpImm); }
  inline bool hasLabelOp() const noexcept { return hasOpType(Operand::kOpLabel); }

  inline uint32_t indexOfOpType(uint32_t opType) const noexcept {
    uint32_t i = 0;
    uint32_t count = opCount();

    while (i < count) {
      if (_opArray[i].opType() == opType)
        break;
      i++;
    }

    return i;
  }

  inline uint32_t indexOfMemOp() const noexcept { return indexOfOpType(Operand::kOpMem); }
  inline uint32_t indexOfImmOp() const noexcept { return indexOfOpType(Operand::kOpImm); }
  inline uint32_t indexOfLabelOp() const noexcept { return indexOfOpType(Operand::kOpLabel); }

  //! \}

  //! \name Rewriting
  //! \{

  inline uint32_t* _getRewriteArray() noexcept { return &_baseInst._extraReg._id; }
  inline const uint32_t* _getRewriteArray() const noexcept { return &_baseInst._extraReg._id; }

  ASMJIT_INLINE uint32_t getRewriteIndex(const uint32_t* id) const noexcept {
    const uint32_t* array = _getRewriteArray();
    ASMJIT_ASSERT(array <= id);

    size_t index = (size_t)(id - array);
    ASMJIT_ASSERT(index < 32);

    return uint32_t(index);
  }

  ASMJIT_INLINE void rewriteIdAtIndex(uint32_t index, uint32_t id) noexcept {
    uint32_t* array = _getRewriteArray();
    array[index] = id;
  }

  //! \}

  //! \name Static Functions
  //! \{

  static inline uint32_t capacityOfOpCount(uint32_t opCount) noexcept {
    return opCount <= kBaseOpCapacity ? kBaseOpCapacity : Globals::kMaxOpCount;
  }

  static inline size_t nodeSizeOfOpCapacity(uint32_t opCapacity) noexcept {
    size_t base = sizeof(InstNode) - kBaseOpCapacity * sizeof(Operand);
    return base + opCapacity * sizeof(Operand);
  }

  //! \}
};

// ============================================================================
// [asmjit::InstExNode]
// ============================================================================

//! Instruction node with maximum number of operands..
//!
//! This node is created automatically by Builder/Compiler in case that the
//! required number of operands exceeds the default capacity of `InstNode`.
class InstExNode : public InstNode {
public:
  ASMJIT_NONCOPYABLE(InstExNode)

  //! Continued `_opArray[]` to hold up to `kMaxOpCount` operands.
  Operand_ _opArrayEx[Globals::kMaxOpCount - kBaseOpCapacity];

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `InstExNode` instance.
  inline InstExNode(BaseBuilder* cb, uint32_t instId, uint32_t options, uint32_t opCapacity = Globals::kMaxOpCount) noexcept
    : InstNode(cb, instId, options, opCapacity) {}

  //! \}
};

// ============================================================================
// [asmjit::SectionNode]
// ============================================================================

//! Section node.
class SectionNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(SectionNode)

  //! Section id.
  uint32_t _id;

  //! Next section node that follows this section.
  //!
  //! This link is only valid when the section is active (is part of the code)
  //! and when `Builder::hasDirtySectionLinks()` returns `false`. If you intend
  //! to use this field you should always call `Builder::updateSectionLinks()`
  //! before you do so.
  SectionNode* _nextSection;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `SectionNode` instance.
  inline SectionNode(BaseBuilder* cb, uint32_t id = 0) noexcept
    : BaseNode(cb, kNodeSection, kFlagHasNoEffect),
      _id(id),
      _nextSection(nullptr) {}

  //! \}

  //! \name Accessors
  //! \{

  //! Returns the section id.
  inline uint32_t id() const noexcept { return _id; }

  //! \}
};

// ============================================================================
// [asmjit::LabelNode]
// ============================================================================

//! Label node.
class LabelNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(LabelNode)

  uint32_t _id;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `LabelNode` instance.
  inline LabelNode(BaseBuilder* cb, uint32_t id = 0) noexcept
    : BaseNode(cb, kNodeLabel, kFlagHasNoEffect | kFlagActsAsLabel),
      _id(id) {}

  //! \}

  //! \name Accessors
  //! \{

  //! Returns the id of the label.
  inline uint32_t id() const noexcept { return _id; }
  //! Returns the label as `Label` operand.
  inline Label label() const noexcept { return Label(_id); }

  //! \}
};

// ============================================================================
// [asmjit::AlignNode]
// ============================================================================

//! Align directive (BaseBuilder).
//!
//! Wraps `.align` directive.
class AlignNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(AlignNode)

  //! Align mode, see `AlignMode`.
  uint32_t _alignMode;
  //! Alignment (in bytes).
  uint32_t _alignment;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `AlignNode` instance.
  inline AlignNode(BaseBuilder* cb, uint32_t alignMode, uint32_t alignment) noexcept
    : BaseNode(cb, kNodeAlign, kFlagIsCode | kFlagHasNoEffect),
      _alignMode(alignMode),
      _alignment(alignment) {}

  //! \}

  //! \name Accessors
  //! \{

  //! Returns align mode.
  inline uint32_t alignMode() const noexcept { return _alignMode; }
  //! Sets align mode to `alignMode`.
  inline void setAlignMode(uint32_t alignMode) noexcept { _alignMode = alignMode; }

  //! Returns align offset in bytes.
  inline uint32_t alignment() const noexcept { return _alignment; }
  //! Sets align offset in bytes to `offset`.
  inline void setAlignment(uint32_t alignment) noexcept { _alignment = alignment; }

  //! \}
};

// ============================================================================
// [asmjit::EmbedDataNode]
// ============================================================================

//! Embed data node.
//!
//! Wraps `.data` directive. The node contains data that will be placed at the
//! node's position in the assembler stream. The data is considered to be RAW;
//! no analysis nor byte-order conversion is performed on RAW data.
class EmbedDataNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(EmbedDataNode)

  enum : uint32_t {
    kInlineBufferSize = uint32_t(64 - sizeof(BaseNode) - 4)
  };

  union {
    struct {
      //! Embedded data buffer.
      uint8_t _buf[kInlineBufferSize];
      //! Size of the data.
      uint32_t _size;
    };
    struct {
      //! Pointer to external data.
      uint8_t* _externalPtr;
    };
  };

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `EmbedDataNode` instance.
  inline EmbedDataNode(BaseBuilder* cb, void* data, uint32_t size) noexcept
    : BaseNode(cb, kNodeEmbedData, kFlagIsData) {

    if (size <= kInlineBufferSize) {
      if (data)
        memcpy(_buf, data, size);
    }
    else {
      _externalPtr = static_cast<uint8_t*>(data);
    }
    _size = size;
  }

  //! \}

  //! \name Accessors
  //! \{

  //! Returns pointer to the data.
  inline uint8_t* data() const noexcept { return _size <= kInlineBufferSize ? const_cast<uint8_t*>(_buf) : _externalPtr; }
  //! Returns size of the data.
  inline uint32_t size() const noexcept { return _size; }

  //! \}
};

// ============================================================================
// [asmjit::EmbedLabelNode]
// ============================================================================

//! Label data node.
class EmbedLabelNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(EmbedLabelNode)

  uint32_t _id;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `EmbedLabelNode` instance.
  inline EmbedLabelNode(BaseBuilder* cb, uint32_t id = 0) noexcept
    : BaseNode(cb, kNodeEmbedLabel, kFlagIsData),
      _id(id) {}

  //! \}

  //! \name Accessors
  //! \{

  //! Returns the id of the label.
  inline uint32_t id() const noexcept { return _id; }
  //! Sets the label id (use with caution, improper use can break a lot of things).
  inline void setId(uint32_t id) noexcept { _id = id; }

  //! Returns the label as `Label` operand.
  inline Label label() const noexcept { return Label(_id); }
  //! Sets the label id from `label` operand.
  inline void setLabel(const Label& label) noexcept { setId(label.id()); }

  //! \}
};

// ============================================================================
// [asmjit::EmbedLabelDeltaNode]
// ============================================================================

//! Label data node.
class EmbedLabelDeltaNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(EmbedLabelDeltaNode)

  uint32_t _id;
  uint32_t _baseId;
  uint32_t _dataSize;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `EmbedLabelDeltaNode` instance.
  inline EmbedLabelDeltaNode(BaseBuilder* cb, uint32_t id = 0, uint32_t baseId = 0, uint32_t dataSize = 0) noexcept
    : BaseNode(cb, kNodeEmbedLabelDelta, kFlagIsData),
      _id(id),
      _baseId(baseId),
      _dataSize(dataSize) {}

  //! \}

  //! \name Accessors
  //! \{

  //! Returns the id of the label.
  inline uint32_t id() const noexcept { return _id; }
  //! Sets the label id.
  inline void setId(uint32_t id) noexcept { _id = id; }
  //! Returns the label as `Label` operand.
  inline Label label() const noexcept { return Label(_id); }
  //! Sets the label id from `label` operand.
  inline void setLabel(const Label& label) noexcept { setId(label.id()); }

  //! Returns the id of the base label.
  inline uint32_t baseId() const noexcept { return _baseId; }
  //! Sets the base label id.
  inline void setBaseId(uint32_t baseId) noexcept { _baseId = baseId; }
  //! Returns the base label as `Label` operand.
  inline Label baseLabel() const noexcept { return Label(_baseId); }
  //! Sets the base label id from `label` operand.
  inline void setBaseLabel(const Label& baseLabel) noexcept { setBaseId(baseLabel.id()); }

  inline uint32_t dataSize() const noexcept { return _dataSize; }
  inline void setDataSize(uint32_t dataSize) noexcept { _dataSize = dataSize; }

  //! \}
};

// ============================================================================
// [asmjit::ConstPoolNode]
// ============================================================================

//! A node that wraps `ConstPool`.
class ConstPoolNode : public LabelNode {
public:
  ASMJIT_NONCOPYABLE(ConstPoolNode)

  ConstPool _constPool;

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `ConstPoolNode` instance.
  inline ConstPoolNode(BaseBuilder* cb, uint32_t id = 0) noexcept
    : LabelNode(cb, id),
      _constPool(&cb->_codeZone) {

    setType(kNodeConstPool);
    addFlags(kFlagIsData);
    clearFlags(kFlagIsCode | kFlagHasNoEffect);
  }

  //! \}

  //! \name Accessors
  //! \{

  //! Tests whether the constant-pool is empty.
  inline bool empty() const noexcept { return _constPool.empty(); }
  //! Returns the size of the constant-pool in bytes.
  inline size_t size() const noexcept { return _constPool.size(); }
  //! Returns minimum alignment.
  inline size_t alignment() const noexcept { return _constPool.alignment(); }

  //! Returns the wrapped `ConstPool` instance.
  inline ConstPool& constPool() noexcept { return _constPool; }
  //! Returns the wrapped `ConstPool` instance (const).
  inline const ConstPool& constPool() const noexcept { return _constPool; }

  //! \}

  //! \name Utilities
  //! \{

  //! See `ConstPool::add()`.
  inline Error add(const void* data, size_t size, size_t& dstOffset) noexcept {
    return _constPool.add(data, size, dstOffset);
  }

  //! \}
};

// ============================================================================
// [asmjit::CommentNode]
// ============================================================================

//! Comment node.
class CommentNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(CommentNode)

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `CommentNode` instance.
  inline CommentNode(BaseBuilder* cb, const char* comment) noexcept
    : BaseNode(cb, kNodeComment, kFlagIsInformative | kFlagHasNoEffect | kFlagIsRemovable) {
    _inlineComment = comment;
  }

  //! \}
};

// ============================================================================
// [asmjit::SentinelNode]
// ============================================================================

//! Sentinel node.
//!
//! Sentinel is a marker that is completely ignored by the code builder. It's
//! used to remember a position in a code as it never gets removed by any pass.
class SentinelNode : public BaseNode {
public:
  ASMJIT_NONCOPYABLE(SentinelNode)

  //! Type of the sentinel (purery informative purpose).
  enum SentinelType : uint32_t {
    kSentinelUnknown = 0u,
    kSentinelFuncEnd = 1u
  };

  //! \name Construction & Destruction
  //! \{

  //! Creates a new `SentinelNode` instance.
  inline SentinelNode(BaseBuilder* cb, uint32_t sentinelType = kSentinelUnknown) noexcept
    : BaseNode(cb, kNodeSentinel, kFlagIsInformative | kFlagHasNoEffect) {

    _sentinel._sentinelType = uint8_t(sentinelType);
  }

  //! \}

  //! \name Accessors
  //! \{

  inline uint32_t sentinelType() const noexcept { return _sentinel._sentinelType; }
  inline void setSentinelType(uint32_t type) noexcept { _sentinel._sentinelType = uint8_t(type); }

  //! \}
};

// ============================================================================
// [asmjit::Pass]
// ============================================================================

//! Pass can be used to implement code transformations, analysis, and lowering.
class ASMJIT_VIRTAPI Pass {
public:
  ASMJIT_BASE_CLASS(Pass)
  ASMJIT_NONCOPYABLE(Pass)

  //! BaseBuilder this pass is assigned to.
  BaseBuilder* _cb;
  //! Name of the pass.
  const char* _name;

  //! \name Construction & Destruction
  //! \{

  ASMJIT_API Pass(const char* name) noexcept;
  ASMJIT_API virtual ~Pass() noexcept;

  //! \}

  //! \name Accessors
  //! \{

  inline const BaseBuilder* cb() const noexcept { return _cb; }
  inline const char* name() const noexcept { return _name; }

  //! \}

  //! \name Pass Interface
  //! \{

  //! Processes the code stored in Builder or Compiler.
  //!
  //! This is the only function that is called by the `BaseBuilder` to process
  //! the code. It passes `zone`, which will be reset after the `run()` finishes.
  virtual Error run(Zone* zone, Logger* logger) noexcept = 0;

  //! \}
};

//! \}

ASMJIT_END_NAMESPACE

#endif // !ASMJIT_NO_BUILDER
#endif // _ASMJIT_CORE_BUILDER_H
