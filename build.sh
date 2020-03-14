echo "syncing"
rsync -ac --out-format="   %n" --no-t --delete-after --exclude ".vscode/" --exclude "ttymidi.code-workspace" --exclude "build.sh" . root@192.168.7.2:/root/ttymidi/
MAKE_ARG=$1
echo "make $MAKE_ARG"
ssh root@192.168.7.2 "make -C /root/ttymidi/ $MAKE_ARG"