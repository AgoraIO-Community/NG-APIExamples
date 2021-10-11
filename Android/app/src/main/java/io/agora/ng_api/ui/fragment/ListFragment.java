package io.agora.ng_api.ui.fragment;

import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import io.agora.ng_api.base.BaseFragment;
import io.agora.ng_api.databinding.FragmentListBinding;
import io.agora.ng_api.ui.ListAdapter;
import io.agora.ng_api.ui.PinnedTitleDecoration;
import io.agora.ng_api.ui.activity.MainActivity;

/**
 * fragment contains all demo fragment
 */
public class ListFragment extends BaseFragment<FragmentListBinding> {
    private PinnedTitleDecoration mDecoration;
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView();
        if (((MainActivity) requireActivity()).getDemoInfoList().isEmpty()) {
            new Thread(() -> {
                ((MainActivity) requireActivity()).configData();
                new Handler(Looper.getMainLooper()).post(this::doChangeView);
            }).start();
        }
    }

    private void initView() {
        mDecoration = new PinnedTitleDecoration(requireContext(), ((MainActivity) requireActivity()).getDemoInfoList());

        // Finally, on Android 12(S) overScroll no longer have ripple animation
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.R)
            mBinding.recyclerViewFgList.setOverScrollMode(RecyclerView.OVER_SCROLL_NEVER);
        mBinding.recyclerViewFgList.setAdapter(new ListAdapter(((MainActivity)getParentActivity()).getDemoInfoList()));
        mBinding.recyclerViewFgList.addItemDecoration(mDecoration);
    }

    @Override
    public void doChangeView() {
        if(mDecoration!=null) mDecoration.updatePaintColor(requireContext());
        RecyclerView.Adapter<?> adapter = mBinding.recyclerViewFgList.getAdapter();
        if(adapter!=null) adapter.notifyDataSetChanged();

    }
}
