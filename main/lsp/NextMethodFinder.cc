#include "NextMethodFinder.h"
#include "ast/treemap/treemap.h"
#include "core/GlobalState.h"

using namespace std;

namespace sorbet::realmain::lsp {

void NextMethodFinder::preTransformClassDef(core::Context ctx, ast::ExpressionPtr &tree) {
    auto &classDef = ast::cast_tree_nonnull<ast::ClassDef>(tree);

    auto loc = ctx.locAt(tree.loc());

    if (!this->narrowestClassDefRange.exists()) {
        // No narrowestClassDefRange yet, so take the <root> loc
        ENFORCE(classDef.symbol == core::Symbols::root());
        this->narrowestClassDefRange = loc;
    } else if (loc.contains(this->queryLoc) && this->narrowestClassDefRange.contains(loc)) {
        // `loc` is contained in the current narrowestClassDefRange, and still contains `queryLoc`
        this->narrowestClassDefRange = loc;

        if (this->result_.second.exists() && !loc.contains(this->result_.first)) {
            // If there's a result and it's not contained in the new narrowest range, we have to toss it out
            // (Method defs and class defs are not necessarily sorted by their locs)
            this->result_ = {core::Loc::none(), core::Symbols::noMethod()};
        }
    }

    this->scopeContainsQueryLoc.emplace_back(loc.contains(this->queryLoc));
}

void NextMethodFinder::postTransformClassDef(core::Context ctx, ast::ExpressionPtr &tree) {
    ENFORCE(!this->scopeContainsQueryLoc.empty());
    this->scopeContainsQueryLoc.pop_back();
}

void NextMethodFinder::preTransformMethodDef(core::Context ctx, ast::ExpressionPtr &tree) {
    auto &methodDef = ast::cast_tree_nonnull<ast::MethodDef>(tree);
    ENFORCE(methodDef.symbol.exists());
    ENFORCE(methodDef.symbol != core::Symbols::todoMethod());

    ENFORCE(!this->scopeContainsQueryLoc.empty());
    if (!this->scopeContainsQueryLoc.back()) {
        // Regardless of whether this method is after the queryLoc or inside the narrowestClassDefRange,
        // we're in a ClassDef whose scope doesn't contain the queryLoc.
        // (one case where this happens: nested Inner class)
        return;
    }

    auto currentMethod = methodDef.symbol;

    auto currentLoc = ctx.locAt(tree.loc());
    if (!currentLoc.exists()) {
        // Defensive in case location information is disabled (e.g., certain fuzzer modes)
        return;
    }

    ENFORCE(this->narrowestClassDefRange.exists());

    if (!this->narrowestClassDefRange.contains(currentLoc)) {
        // This method occurs outside the current narrowest range we know of for a ClassDef that
        // still contains queryLoc, so even if this MethodDef is after the queryLoc, it would not be
        // in the right scope.
        return;
    } else if (currentLoc.beginPos() < queryLoc.beginPos()) {
        // Current method is before query, not after.
        return;
    }

    // Current method starts at or after query loc. Starting 'at' is fine, because it can happen in cases like this:
    //   |def foo; end

    if (this->result_.second.exists()) {
        // Method defs are not guaranteed to be sorted in order by their declLocs
        auto resultLoc = this->result_.first;
        if (currentLoc.beginPos() < resultLoc.beginPos()) {
            // Found a method defined after the query but earlier than previous result: overwrite previous result
            this->result_ = {currentLoc, currentMethod};
            return;
        } else {
            // We've already found an earlier result, so the current is not the first
            return;
        }
    } else {
        // Haven't found a result yet, so this one is the best so far.
        this->result_ = {currentLoc, currentMethod};
        return;
    }
}

core::MethodRef NextMethodFinder::firstMethodAfterQuery(const core::GlobalState &gs, ast::ExpressionPtr &tree,
                                                        core::Loc queryLoc) {
    NextMethodFinder nextMethodFinder(queryLoc);
    auto ctx = core::Context(gs, core::Symbols::root(), queryLoc.file());
    ast::TreeWalk::apply(ctx, nextMethodFinder, tree);

    return nextMethodFinder.result_.second;
}

}; // namespace sorbet::realmain::lsp
