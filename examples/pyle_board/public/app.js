let boardData = null;
let draggedCardId = null;
let sourceLaneId = null;

async function loadBoard() {
    const res = await fetch('/api/board');
    boardData = await res.json();
    renderBoard();
}

function renderBoard() {
    const boardEl = document.getElementById('board');
    boardEl.innerHTML = '';
    const laneSelect = document.getElementById('newCardLane');
    laneSelect.innerHTML = '';

    boardData.lanes.forEach(lane => {
        const option = document.createElement('option');
        option.value = lane.id;
        option.textContent = lane.title;
        laneSelect.appendChild(option);

        const laneEl = document.createElement('div');
        laneEl.className = 'lane';
        laneEl.dataset.laneId = lane.id;
        laneEl.addEventListener('dragover', handleDragOver);
        laneEl.addEventListener('drop', handleDrop);

        // header
        const header = document.createElement('div');
        header.className = 'lane-header';
        header.innerHTML = `<span>${lane.title}</span><span style="color:#6b778c; font-size: 0.8em;">${lane.cards.length}</span>`;
        laneEl.appendChild(header);

        // cards container
        const cardsEl = document.createElement('div');
        cardsEl.className = 'cards-container';
        
        lane.cards.forEach(card => {
            const cardEl = document.createElement('div');
            cardEl.className = 'card';
            cardEl.draggable = true;
            cardEl.dataset.cardId = card.id;
            
            if (card.tag && card.tag !== 'None') {
                const tagEl = document.createElement('div');
                tagEl.className = `card-tag tag-${card.tag}`;
                cardEl.appendChild(tagEl);
            }

            const textEl = document.createElement('div');
            textEl.className = 'card-text';
            textEl.textContent = card.text;
            cardEl.appendChild(textEl);

            cardEl.addEventListener('dragstart', (e) => {
                draggedCardId = card.id;
                sourceLaneId = lane.id;
                cardEl.classList.add('dragging');
                e.dataTransfer.effectAllowed = 'move';
            });
            
            cardEl.addEventListener('dragend', () => {
                cardEl.classList.remove('dragging');
                draggedCardId = null;
                sourceLaneId = null;
            });

            cardsEl.appendChild(cardEl);
        });

        laneEl.appendChild(cardsEl);
        boardEl.appendChild(laneEl);
    });
}

function handleDragOver(e) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
}

async function handleDrop(e) {
    e.preventDefault();
    const laneEl = e.target.closest('.lane');
    if (!laneEl) return;
    
    const targetLaneId = laneEl.dataset.laneId;
    
    if (sourceLaneId === targetLaneId) return; // Optimization: don't call API if dropping in same lane (simplification)

    const res = await fetch('/api/cards/move', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            cardId: draggedCardId,
            sourceLaneId: sourceLaneId,
            targetLaneId: targetLaneId
        })
    });

    if (res.ok) {
        loadBoard();
    } else {
        alert('Failed to move card');
    }
}

function showAddCardModal() {
    document.getElementById('addCardModal').classList.add('open');
}

function closeModal() {
    document.getElementById('addCardModal').classList.remove('open');
}

async function handleAddCard(e) {
    e.preventDefault();
    const laneId = document.getElementById('newCardLane').value;
    const text = document.getElementById('newCardText').value;
    const tag = document.getElementById('newCardTag').value;

    const res = await fetch('/api/cards', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ laneId, text, tag })
    });

    if (res.ok) {
        closeModal();
        e.target.reset();
        loadBoard();
    } else {
        alert('Error creating card');
    }
}

loadBoard();
