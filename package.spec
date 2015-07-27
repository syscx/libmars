Name: scg-libmars
License: Ruckus
Summary: libmars
Version: %{rks_release_version}
Release: %{rks_build_no}
Vendor: Ruckus%{?changelist:<CL#%{changelist}>}

%description

%prep
rm -rf %{_builddir}
mkdir -p %{_builddir}

%build
make -C %{rks_package_dir} MARS_BIN_DIR=%{_builddir}

%install
mkdir -p %{buildroot}%{_includedir}
install -t %{buildroot}%{_includedir}/ %{rks_package_dir}/include/*.h 
install -D %{_builddir}/libmars.so %{buildroot}%{_libdir}/libmars.so

%if %{getenv:RKS_DEBUGINFO}0
%debug_package
%endif

%files
%defattr(-,root,root,-)
%{_includedir}/*.h
%{_libdir}/libmars.so
