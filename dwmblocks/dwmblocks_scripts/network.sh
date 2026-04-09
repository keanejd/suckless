IFACE=$(ip route get 1 | awk '{for(i=1;i<=NF;i++) if($i=="dev") print $(i+1); exit}')
IP=$(ip route get 1 | awk '{print $7; exit}')

if [[ "$IFACE" == w* ]]; then
    ICON="󰖩"   # nerd font wifi icon
else
    ICON="󰈀"   # nerd font ethernet icon
fi

echo "$ICON $IP"
